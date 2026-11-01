#include "tutorialcontroller.h"

#include "subtitlegenerator.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSettings>
#include <QUrl>

#include <algorithm>

TutorialController::TutorialController(QObject* parent)
    : QObject(parent)
    , m_effects(new VideoEffectModel(this))
    , m_tts(new TtsEngine(this))
    , m_renderer(new VideoRenderer(this))
{
    connect(m_tts, &TtsEngine::generationFinished, this, &TutorialController::onGenerationFinished);
    // Keep the TTS triggers in sync with the contents of the effect model.
    connect(m_effects, &VideoEffectModel::rowsInserted, this, [this] { connectEvents(); });
    connect(m_effects, &VideoEffectModel::modelReset, this, [this] { connectEvents(); });
    // When the Piper environment changes (script path re-derives it, or the
    // model directory is set explicitly), re-expose the available languages
    // and voices and re-validate the current voice against them.
    const auto refreshVoices= [this] {
        const auto voices= m_tts->voicesForLanguage(m_language);
        if(voices.isEmpty())
            setVoice(QString());
        else if(!voices.contains(m_voice))
            setVoice(voices.first());
        emit languagesChanged();
        emit voicesChanged();
    };
    connect(m_tts, &TtsEngine::scriptPathChanged, this, refreshVoices);
    connect(m_tts, &TtsEngine::modelDirectoryChanged, this, refreshVoices);
    loadSettings();
}

TutorialController::~TutorialController()= default;

VideoEffectModel* TutorialController::effects() const
{
    return m_effects;
}

TtsEngine* TutorialController::ttsEngine() const
{
    return m_tts;
}

VideoRenderer* TutorialController::renderer() const
{
    return m_renderer;
}

QString TutorialController::videoFilePath() const
{
    return m_videoFilePath;
}

void TutorialController::setVideoFilePath(const QString& path)
{
    if(m_videoFilePath == path)
        return;
    m_videoFilePath= path;
    emit videoFilePathChanged();
}

QString TutorialController::projectFilePath() const
{
    return m_projectFilePath;
}

void TutorialController::setProjectFilePath(const QString& path)
{
    if(m_projectFilePath == path)
        return;
    m_projectFilePath= path;
    emit projectFilePathChanged();
}

int TutorialController::stage() const
{
    return m_stage;
}

int TutorialController::current() const
{
    return m_current;
}

int TutorialController::total() const
{
    return m_total;
}

QString TutorialController::statusMessage() const
{
    return m_statusMessage;
}

bool TutorialController::busy() const
{
    return m_busy;
}

QString TutorialController::srtPath() const
{
    return m_srtPath;
}

QString TutorialController::outputVideoPath() const
{
    return m_outputVideoPath;
}

qint64 TutorialController::currentPositionMs() const
{
    return m_currentPositionMs;
}

void TutorialController::setCurrentPositionMs(qint64 positionMs)
{
    if(m_currentPositionMs == positionMs)
        return;
    m_currentPositionMs= positionMs;
    emit currentPositionMsChanged();
}

// --- TTS settings ---

QString TutorialController::language() const
{
    return m_language;
}

void TutorialController::setLanguage(const QString& lang)
{
    if(m_language == lang)
        return;
    m_language= lang;
    // Update the voice list for the new language; keep the voice if it is
    // compatible, otherwise fall back to the first available voice.
    const auto voices= m_tts->voicesForLanguage(lang);
    emit languageChanged();
    if(!voices.contains(m_voice))
        setVoice(voices.isEmpty() ? QString() : voices.first());
    else
        emit voicesChanged();
    writeSettings();
}

QString TutorialController::voice() const
{
    return m_voice;
}

void TutorialController::setVoice(const QString& voice)
{
    if(m_voice == voice)
        return;
    m_voice= voice;
    emit voiceChanged();
    writeSettings();
}

QStringList TutorialController::languages() const
{
    return m_tts->languages();
}

QStringList TutorialController::voices() const
{
    return m_tts->voicesForLanguage(m_language);
}

void TutorialController::addCommentAtCurrentPosition(qint64 start, qint64 end)
{
    m_effects->addEffect(VideoTimedEvent::EventType::AUDIOCOMMENT, start, end);
}

void TutorialController::addCommentWithText(qint64 start, qint64 end, const QString& text)
{
    m_effects->addEffect(VideoTimedEvent::EventType::AUDIOCOMMENT, start, end);
    auto* comment= qobject_cast<AudioCommentEvent*>(m_effects->effectAt(m_effects->rowCount() - 1));
    if(comment)
        comment->setText(text);
}

void TutorialController::addSpeedEffect(qint64 start, qint64 end)
{
    m_effects->addEffect(VideoTimedEvent::EventType::SPEEDUPEVENT, start, end);
}

void TutorialController::addCutEffect(qint64 start, qint64 end)
{
    m_effects->addEffect(VideoTimedEvent::EventType::CUTEVENT, start, end);
}

void TutorialController::removeEffect(int id)
{
    m_effects->removeEffect(id);
}

AudioCommentEvent* TutorialController::findComment(const QString& id) const
{
    for(const auto& e : m_effects->allEffects())
    {
        if(auto* c= qobject_cast<AudioCommentEvent*>(e.get()))
        {
            if(c->id() == id)
                return c;
        }
    }
    return nullptr;
}

void TutorialController::regenerateAudio(const QString& commentId)
{
    auto* comment= findComment(commentId);
    if(!comment)
        return;
    if(m_busy)
    {
        setStatus("Already generating audio.");
        return;
    }

    QString details;
    if(!m_tts->isAvailable(&details))
    {
        setStatus("Piper is not available:\n" + details);
        emit workflowFinished(false, "Piper is not available: " + details);
        return;
    }

    // Drop the old file so the outdated audio is not played while the new
    // one is being generated.
    if(!comment->audioFile().isEmpty())
    {
        if(QFileInfo::exists(comment->audioFile()))
            QFile::remove(comment->audioFile());
        comment->setAudioFile(QString());
    }

    // Reuse the generation queue for this single comment.
    m_audioQueue= QQueue<QPointer<AudioCommentEvent>>();
    m_audioQueue.enqueue(comment);
    m_total= 1;
    m_current= 0;
    emit progressChanged();
    processNextInQueue();
}

void TutorialController::removeAudio(const QString& commentId)
{
    auto* comment= findComment(commentId);
    if(!comment || comment->audioFile().isEmpty())
        return;

    const QString fileName= QFileInfo(comment->audioFile()).fileName();
    if(QFileInfo::exists(comment->audioFile()))
        QFile::remove(comment->audioFile());
    comment->setAudioFile(QString());
    setStatus("Audio removed: " + fileName);
}

QString TutorialController::workingDirectory() const
{
    // Generated assets live next to the project file so the project stays
    // portable. When no project file is set yet, use a stable fallback.
    QString base;
    if(!m_projectFilePath.isEmpty())
        base= QFileInfo(m_projectFilePath).absolutePath();
    else
        base= QDir::homePath();

    const QString projectName
        = m_projectFilePath.isEmpty() ? "ClipSelectorTutorial" : QFileInfo(m_projectFilePath).completeBaseName();

    return QDir(base).filePath(projectName + ".tutorial");
}

QString TutorialController::audioDirFor(const QString& commentId) const
{
    return QDir(workingDirectory()).filePath("audio/" + commentId);
}

void TutorialController::generateAudio()
{
    if(m_busy)
        return;

    QString details;
    if(!m_tts->isAvailable(&details))
    {
        setStatus("Piper is not available:\n" + details);
        emit workflowFinished(false, "Piper is not available: " + details);
        return;
    }

    // Collect comments that need (re)generation: they have text but no audio
    // file yet (e.g. the project was loaded without generated assets).
    m_audioQueue= QQueue<QPointer<AudioCommentEvent>>();
    for(const auto& e : m_effects->allEffects())
    {
        if(auto* c= qobject_cast<AudioCommentEvent*>(e.get()))
        {
            if(!c->text().isEmpty() && c->audioFile().isEmpty())
                m_audioQueue.enqueue(c);
        }
    }

    if(m_audioQueue.isEmpty())
    {
        setStatus("All comments already have up-to-date audio.");
        emit workflowFinished(true, "No audio needed: all comments are up to date.");
        return;
    }

    m_total= m_audioQueue.size();
    m_current= 0;
    emit progressChanged();
    processNextInQueue();
}

void TutorialController::processNextInQueue()
{
    QPointer<AudioCommentEvent> event;
    while(!m_audioQueue.isEmpty())
    {
        event= m_audioQueue.dequeue();
        if(event)
            break;
    }

    if(!event)
    {
        // Only report success when a generation was actually in flight; a
        // Done stage already carries a failure message that must not be
        // overwritten.
        if(m_stage == GeneratingAudio)
        {
            setStage(Done);
            setBusy(false);
            setStatus("Audio generation finished.");
            emit workflowFinished(true, "Audio generation finished.");
        }
        return;
    }

    if(event->text().isEmpty())
    {
        processNextInQueue();
        return;
    }

    QString details;
    if(!m_tts->isAvailable(&details))
    {
        setStatus("Piper is not available:\n" + details);
        // Drain the rest of the queue so the UI is not left busy.
        m_audioQueue= QQueue<QPointer<AudioCommentEvent>>();
        m_generatingEvent.clear();
        setStage(Done);
        setBusy(false);
        emit workflowFinished(false, "Piper is not available: " + details);
        return;
    }

    m_total= m_audioQueue.size() + 1;
    ++m_current;
    setStage(GeneratingAudio);
    setBusy(true);
    setStatus(QString("Generating audio (%1/%2): %3").arg(m_current).arg(m_total).arg(event->text().left(40)));
    emit progressChanged();

    m_generatingEvent= event;
    m_tts->ensureDestDir(audioDirFor(event->id()));
    m_tts->generateAsync(event->text(), audioDirFor(event->id()), m_language);
}

void TutorialController::onGenerationFinished(const QString& text, const TtsEngine::GenerationResult& result)
{
    Q_UNUSED(text);
    // Correlate the async result back to the comment we started generating.
    QPointer<AudioCommentEvent> event= m_generatingEvent;
    m_generatingEvent.clear();

    if(event)
    {
        if(result.ok)
        {
            event->setAudioFile(result.audioFile);
            // Adapt the event duration to the length of the generated audio.
            event->setEnd(event->begin() + result.durationMs);
        }
        else
        {
            setStage(Done);
            setBusy(false);
            setStatus("Audio generation failed: " + result.error);
            emit workflowFinished(false, "Audio generation failed: " + result.error);
        }
    }

    processNextInQueue();
}

void TutorialController::connectEvents()
{
    for(const auto& e : m_effects->allEffects())
    {
        auto* comment= qobject_cast<AudioCommentEvent*>(e.get());
        if(!comment)
            continue;
        // Idempotent: drop any previous trigger for this comment first.
        QObject::disconnect(comment, nullptr, this, nullptr);
        connect(comment, &AudioCommentEvent::textChanged, this,
                [this, comment]
                {
                    if(comment->text().isEmpty())
                        return;
                    // Avoid double-queueing the same comment.
                    for(const auto& queued : std::as_const(m_audioQueue))
                    {
                        if(queued == comment)
                            return;
                    }
                    m_audioQueue.enqueue(comment);
                    if(!m_busy)
                        processNextInQueue();
                });
    }
}

void TutorialController::setStage(Stage stage)
{
    if(m_stage == stage)
        return;
    m_stage= stage;
    emit stageChanged();
}

void TutorialController::setStatus(const QString& message)
{
    if(m_statusMessage == message)
        return;
    m_statusMessage= message;
    emit statusMessageChanged();
}

void TutorialController::setBusy(bool busy)
{
    if(m_busy == busy)
        return;
    m_busy= busy;
    emit busyChanged();
}

void TutorialController::generateSubtitle()
{
    /*if(m_comments->allComments().isEmpty())
    {
        setStatus("No comments to subtitle.");
        emit workflowFinished(false, "No comments to subtitle.");
        return;
    }

    setStage(GeneratingSubtitles);
    setBusy(true);

    QList<SubtitleGenerator::Entry> entries;
    for(const auto* c : m_comments->sortedByTime())
    {
        SubtitleGenerator::Entry e;
        e.text= c->text();
        e.startMs= c->startTimeMs();
        e.endMs= c->startTimeMs() + c->audioDurationMs();
        entries.append(e);
    }

    QStringList problems;
    const QByteArray srt= SubtitleGenerator::generateSrt(entries, &problems);

    m_srtPath= QDir(workingDirectory()).filePath("subtitles.srt");
    QString error;
    if(!SubtitleGenerator::writeSrt(m_srtPath, srt, &error))
    {
        setStage(Done);
        setBusy(false);
        setStatus("Failed to write subtitles: " + error);
        emit workflowFinished(false, "Failed to write subtitles: " + error);
        return;
    }

    emit srtPathChanged();
    setStage(Done);
    setBusy(false);
    setStatus("Subtitles written to " + m_srtPath + (problems.isEmpty() ? QString() : "\n" + problems.join("\n")));
    emit workflowFinished(true, "Subtitles generated: " + m_srtPath);*/
}

void TutorialController::renderVideo(bool keepOriginalAudio, bool embedSubtitles)
{
    if(m_busy)
    {
        setStatus("Already busy.");
        return;
    }

    if(m_videoFilePath.isEmpty())
    {
        setStatus("No source video loaded.");
        emit workflowFinished(false, "No source video loaded.");
        return;
    }

    // Ensure the SRT exists (regenerate if missing) so embedding has a source.
    if(m_srtPath.isEmpty() || !QFileInfo::exists(m_srtPath))
        generateSubtitle();
    if(m_srtPath.isEmpty())
    {
        setStatus("No subtitles available for rendering.");
        emit workflowFinished(false, "No subtitles available for rendering.");
        return;
    }

    // Collect ready narration, in timeline order.
    /*QList<VideoRenderer::CommentAudio> audios;
    bool missing= false;
    for(const auto* c : m_comments->sortedByTime())
    {
        if(c->status() != TutorialComment::Ready || c->audioFile().isEmpty())
        {
            missing= true;
            continue;
        }
        VideoRenderer::CommentAudio a;
        a.audioFile= c->audioFile();
        a.startMs= c->startTimeMs();
        audios.append(a);
    }*/

    /*if(missing)
    {
        setStatus("Some comments have no audio. Generate audio before rendering.");
        emit workflowFinished(false, "Some comments have no audio. Generate audio before rendering.");
        return;
    }
    if(audios.isEmpty())
    {
        setStatus("No narration to render.");
        emit workflowFinished(false, "No narration to render.");
        return;
    }*/

    m_outputVideoPath= QDir(workingDirectory()).filePath("tutorial_output.mp4");
    emit outputVideoPathChanged();

    VideoRenderer::RenderOptions opts;
    opts.sourceVideo= m_videoFilePath;
    opts.outputVideo= m_outputVideoPath;
    opts.srtFile= embedSubtitles ? m_srtPath : QString();
    opts.keepOriginalAudio= keepOriginalAudio;
    opts.embedSubtitles= embedSubtitles;

    setStage(Rendering);
    setBusy(true);
    setStatus("Rendering final video...");

    connect(
        m_renderer, &VideoRenderer::renderFinished, this,
        [this](bool success, const QString& message)
        {
            setStage(Done);
            setBusy(false);
            setStatus(message);
            emit workflowFinished(success, message);
        },
        Qt::UniqueConnection);
    // m_renderer->renderAsync(opts, audios);
}

// --- Project persistence ---

void TutorialController::saveProject(const QString& path)
{
    QJsonObject root;
    root["projectType"]= "tutorial";
    root["video"]= m_videoFilePath;

    QJsonArray effects;
    int i= 0;
    for(const auto& e : m_effects->allEffects())
    {
        QJsonObject obj;
        obj["id"]= i;
        obj["type"]= static_cast<int>(e->type());
        obj["begin"]= e->begin();
        obj["end"]= e->end();
        e->saveData(obj);
        effects.append(obj);
        ++i;
    }
    root["effects"]= effects;

    QString filePath= path;
    if(!filePath.endsWith(".json"))
        filePath+= ".json";

    QSaveFile file(filePath);
    if(!file.open(QIODevice::WriteOnly))
    {
        setStatus("Could not save project: " + file.errorString());
        return;
    }
    file.write(QJsonDocument(root).toJson());
    if(!file.commit())
        setStatus("Could not save project: " + file.errorString());
    else
        setStatus("Project saved: " + filePath);
}

void TutorialController::loadProject(const QString& path)
{
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly))
    {
        setStatus("Could not open project: " + path);
        return;
    }
    QJsonDocument doc= QJsonDocument::fromJson(file.readAll());
    auto obj= doc.object();

    setProjectFilePath(path);
    setVideoFilePath(obj["video"].toString());

    const QString workDir= workingDirectory();
    auto array= obj["comments"].toArray();

    QList<VideoTimedEvent*> effectInfos;
    for(const auto& ref : obj["effects"].toArray())
    {
        auto json= ref.toObject();

        auto type= static_cast<VideoTimedEvent::EventType>(json["type"].toInt());
        auto start= static_cast<qint64>(json["begin"].toDouble());
        auto end= static_cast<qint64>(json["end"].toDouble());
        auto* node= VideoEffectModel::createEvent(type);
        if(!node)
            continue;
        node->setBegin(start);
        node->setEnd(end);
        node->readData(json);

        effectInfos.append(node);
    }
    m_effects->reset(effectInfos);

    m_srtPath= QDir(workDir).filePath("subtitles.srt");
    emit srtPathChanged();
    m_outputVideoPath= QDir(workDir).filePath("tutorial_output.mp4");
    emit outputVideoPathChanged();
    setStatus("Project loaded: " + path);
}

// --- Settings ---

void TutorialController::loadSettings()
{
    QSettings settings("rolisteam", "ClipSelector");
    m_language= settings.value("tutorial/language", "fr").toString();
    m_voice= settings.value("tutorial/voice", QString()).toString();
    m_tts->setScriptPath(settings.value("tutorial/script", m_tts->scriptPath()).toString());
    m_tts->setPythonPath(settings.value("tutorial/python", m_tts->pythonPath()).toString());
    m_tts->setModelDirectory(settings.value("tutorial/modeldir", m_tts->modelDirectory()).toString());

    // If the stored voice is not available for the stored language, fall back
    // to the first compatible voice.
    const auto voices= m_tts->voicesForLanguage(m_language);
    if(!voices.isEmpty() && !voices.contains(m_voice))
        m_voice= voices.first();

    emit languagesChanged();
    emit voicesChanged();
}

void TutorialController::writeSettings() const
{
    QSettings settings("rolisteam", "ClipSelector");
    settings.setValue("tutorial/language", m_language);
    settings.setValue("tutorial/voice", m_voice);
    settings.setValue("tutorial/script", m_tts->scriptPath());
    settings.setValue("tutorial/python", m_tts->pythonPath());
    settings.setValue("tutorial/modeldir", m_tts->modelDirectory());
}
