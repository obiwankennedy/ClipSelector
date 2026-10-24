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
    , m_comments(new TutorialCommentModel(this))
    , m_tts(new TtsEngine(this))
    , m_renderer(new VideoRenderer(this))
{
    connect(m_tts, &TtsEngine::generationFinished, this,
            &TutorialController::onGenerationFinished);
    loadSettings();
}

TutorialController::~TutorialController() = default;

TutorialCommentModel* TutorialController::comments() const
{
    return m_comments;
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

// --- Comment operations ---

QString TutorialController::addComment(const QString& text, qint64 startTimeMs)
{
    if(startTimeMs < 0)
        startTimeMs= m_currentPositionMs;
    return m_comments->addComment(text, startTimeMs, m_language, m_voice);
}

QString TutorialController::addCommentAtCurrentPosition(const QString& text)
{
    return addComment(text, m_currentPositionMs);
}

void TutorialController::removeComment(const QString& id)
{
    const int row= m_comments->indexOfId(id);
    if(row < 0)
        return;

    // Clean up the comment's generated audio only when it is safe: the file is
    // inside our project working directory (never touch source media).
    const auto* c= m_comments->commentAt(row);
    const QString audio= c->audioFile();
    m_comments->removeComment(row);

    if(!audio.isEmpty())
    {
        const QFileInfo fi(audio);
        const QString workDir= workingDirectory();
        if(fi.exists() && workDir.isEmpty() == false
            && audio.startsWith(workDir))
            QFile::remove(audio);
    }
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

    const QString projectName= m_projectFilePath.isEmpty()
        ? "ClipSelectorTutorial"
        : QFileInfo(m_projectFilePath).completeBaseName();

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

    // Collect comments that need (re)generation: pending, outdated or errored.
    m_audioQueue= QQueue<QString>();
    m_pendingText.clear();
    for(const auto* c : m_comments->allComments())
    {
        const int st= c->status();
        if(st == TutorialComment::Pending
            || st == TutorialComment::Outdated
            || st == TutorialComment::Error
            || c->isStale()
            || c->audioFile().isEmpty())
        {
            m_audioQueue.enqueue(c->id());
            m_pendingText.insert(c->id(), c->text());
        }
    }

    if(m_audioQueue.isEmpty())
    {
        setStatus("All comments already have up-to-date audio.");
        emit workflowFinished(true, "No audio needed: all comments are up to date.");
        return;
    }

    setBusy(true);
    setStage(GeneratingAudio);
    m_total= m_audioQueue.size();
    m_current= 0;
    emit progressChanged();
    processNextInQueue();
}

void TutorialController::processNextInQueue()
{
    if(m_audioQueue.isEmpty())
    {
        setStage(Done);
        setBusy(false);
        setStatus("Audio generation finished.");
        emit workflowFinished(true, "Audio generation finished.");
        return;
    }

    const QString id= m_audioQueue.head();
    auto* c= m_comments->commentAt(m_comments->indexOfId(id));
    if(!c)
    {
        m_audioQueue.dequeue();
        processNextInQueue();
        return;
    }

    c->setStatus(TutorialComment::Generating);
    c->setError(QString());
    m_tts->generateAsync(c->text(), audioDirFor(id), c->language());
}

void TutorialController::onGenerationFinished(const QString& text,
                                              const TtsEngine::GenerationResult& result)
{
    // Correlate the async result back to the comment we queued.
    QString id;
    for(auto it= m_pendingText.constBegin(); it != m_pendingText.constEnd(); ++it)
        if(it.value() == text)
        {
            id= it.key();
            break;
        }

    auto* c= id.isEmpty() ? nullptr : m_comments->commentAt(m_comments->indexOfId(id));
    if(!c)
    {
        m_audioQueue.dequeue();
        ++m_current;
        emit progressChanged();
        processNextInQueue();
        return;
    }

    if(result.ok)
    {
        c->setAudioFile(result.audioFile);
        c->setAudioDurationMs(result.durationMs);
        c->markGenerated();
    }
    else
    {
        c->setStatus(TutorialComment::Error);
        c->setError(result.error);
    }

    m_audioQueue.dequeue();
    ++m_current;
    setStatus(QString("Generating audio (%1/%2): %3")
                  .arg(m_current)
                  .arg(m_total)
                  .arg(c->text().left(40)));
    emit progressChanged();
    processNextInQueue();
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
    if(m_comments->allComments().isEmpty())
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
    setStatus("Subtitles written to " + m_srtPath
              + (problems.isEmpty() ? QString() : "\n" + problems.join("\n")));
    emit workflowFinished(true, "Subtitles generated: " + m_srtPath);
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
        return; // generateSubtitle already reported the failure

    // Collect ready narration, in timeline order.
    QList<VideoRenderer::CommentAudio> audios;
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
    }

    if(missing)
    {
        setStatus("Some comments have no audio. Generate audio before rendering.");
        emit workflowFinished(false,
                              "Some comments have no audio. Generate audio before rendering.");
        return;
    }
    if(audios.isEmpty())
    {
        setStatus("No narration to render.");
        emit workflowFinished(false, "No narration to render.");
        return;
    }

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

    connect(m_renderer, &VideoRenderer::renderFinished, this,
            [this](bool success, const QString& message) {
                setStage(Done);
                setBusy(false);
                setStatus(message);
                emit workflowFinished(success, message);
            },
            Qt::UniqueConnection);
    m_renderer->renderAsync(opts, audios);
}


// --- Project persistence ---

void TutorialController::saveProject(const QString& path)
{
    QJsonObject root;
    root["appMode"]= "tutorial";
    root["video"]= m_videoFilePath;

    QJsonArray comments;
    for(const auto* c : m_comments->allComments())
    {
        QJsonObject obj;
        obj["id"]= c->id();
        obj["text"]= c->text();
        obj["startTimeMs"]= static_cast<qint64>(c->startTimeMs());
        obj["audioDurationMs"]= static_cast<qint64>(c->audioDurationMs());
        obj["language"]= c->language();
        obj["voice"]= c->voice();
        obj["status"]= c->status();
        // Store the audio path relative to the project working directory so
        // the project stays portable.
        const QString workDir= workingDirectory();
        const QString audio= c->audioFile();
        if(!audio.isEmpty() && !workDir.isEmpty() && audio.startsWith(workDir))
            obj["audioFile"]= QDir(workDir).relativeFilePath(audio);
        else
            obj["audioFile"]= audio;
        comments.append(obj);
    }
    root["comments"]= comments;

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

    QList<TutorialComment*> infos;
    for(const auto& ref : array)
    {
        auto json= ref.toObject();
        auto c= new TutorialComment(this);
        c->setText(json["text"].toString());
        c->setStartTimeMs(static_cast<qint64>(json["startTimeMs"].toDouble()));
        c->setAudioDurationMs(static_cast<qint64>(json["audioDurationMs"].toDouble()));
        c->setLanguage(json["language"].toString());
        c->setVoice(json["voice"].toString());

        QString audio= json["audioFile"].toString();
        if(!audio.isEmpty() && !audio.startsWith(QDir::homePath())
            && !audio.startsWith("/"))
            audio= QDir(workDir).absoluteFilePath(audio);

        // Detect missing generated audio and flag it for regeneration.
        if(!audio.isEmpty() && QFileInfo::exists(audio))
        {
            c->setAudioFile(audio);
            c->setStatus(static_cast<int>(json["status"].toInt()));
            // Re-snapshot the generation source so isStale() works correctly.
            if(c->status() == TutorialComment::Ready)
                c->markGenerated();
        }
        else
        {
            c->setAudioFile(QString());
            c->setStatus(TutorialComment::Pending);
        }
        infos.append(c);
    }
    m_comments->reset(infos);

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
    m_tts->setScriptPath(settings.value(
        "tutorial/script", m_tts->scriptPath()).toString());
    m_tts->setPythonPath(settings.value(
        "tutorial/python", m_tts->pythonPath()).toString());
    m_tts->setModelDirectory(settings.value(
        "tutorial/modeldir", m_tts->modelDirectory()).toString());

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

