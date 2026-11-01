#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMediaPlayer>
#include <QProcess>
#include <QSoundEffect>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QMouseEvent>
#include <QSignalSpy>
#include <QtTest>

#include "effectplayback.h"
#include "timelineeffecttoken.h"
#include "maincontroller.h"
#include "projectcontroller.h"
#include "subtitlegenerator.h"
#include "ttsengine.h"
#include "tutorialcontroller.h"
#include "utils.h"
#include "videoeffect.h"
#include "videorenderer.h"

class tst_tutorial : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void test_projectTypeNames();
    void test_projectTypeDefaults();
    void test_effectModelCrud();
    void test_effectDataForwarding();
    void test_model2JsonWithEffects();
    void test_piperCommandBuilding();
    void test_ttsPathDerivation();
    void test_ttsFailureDetected();
    void test_settingsPersistence();
    void test_srtValid();
    void test_srtOverlapping();
    void test_srtFallbackDuration();
    void test_audioFilterPlacement();
    void test_originalAudioPreserved();
    void test_rendererMissingInputs();
    void test_projectRoundtrip();
    void test_cancelRender();
    void test_effectPlayback();
    void test_effectModelRoles();
    void test_audioFileManagement();
    void test_tokenMoveUpdatesCommentPosition();
    void cleanupTestCase();

private:
    // Parse an SRT document into (seq, startMs, endMs, text) tuples.
    struct SrtCue
    {
        int seq;
        qint64 startMs;
        qint64 endMs;
        QString text;
    };
    static QList<SrtCue> parseSrt(const QByteArray& data);
    static qint64 parseSrtTimestamp(const QString& ts);
};

void tst_tutorial::initTestCase()
{
    // Use an isolated, freshly-created QSettings scope so tests never
    // clobber (or inherit stale state from) the real settings.
    const QString scope= QCoreApplication::applicationDirPath() + "/tst_settings";
    QDir(scope).removeRecursively();
    qputenv("XDG_CONFIG_HOME", scope.toUtf8());
}

void tst_tutorial::cleanupTestCase()
{
    qunsetenv("XDG_CONFIG_HOME");
}

void tst_tutorial::test_projectTypeNames()
{
    const auto names= ProjectController::projectTypeNames();
    QCOMPARE(names.size(), 2);
    QCOMPARE(names[0], QString::fromUtf8("MUSIC CLIP"));
    QCOMPARE(names[1], QString::fromUtf8("TUTORIAL"));
}

void tst_tutorial::test_projectTypeDefaults()
{
    ProjectController ctrl;
    QCOMPARE(ctrl.projectType(), int(ProjectController::MusicClip));
    ctrl.setProjectType(ProjectController::Tutorial);
    QCOMPARE(ctrl.projectType(), int(ProjectController::Tutorial));
}

void tst_tutorial::test_effectModelCrud()
{
    VideoEffectModel model;
    QCOMPARE(model.rowCount(), 0);

    model.addEffect(VideoTimedEvent::EventType::CUTEVENT, 1000, 1500);
    model.addEffect(VideoTimedEvent::EventType::SPEEDUPEVENT, 2000, 4000);
    model.addEffect(VideoTimedEvent::EventType::AUDIOCOMMENT, 5000, 8000);
    QCOMPARE(model.rowCount(), 3);

    auto* cut= model.effectAt(0);
    auto* speed= model.effectAt(1);
    auto* comment= model.effectAt(2);
    QVERIFY(cut && speed && comment);
    QCOMPARE(cut->begin(), qint64(1000));
    QCOMPARE(cut->end(), qint64(1500));
    QCOMPARE(cut->duration(), qint64(500));

    // SpeedUp: the speed factor is editable (1 = normal, 2 = twice as fast).
    auto* speedUp= qobject_cast<SpeedUpEvent*>(speed);
    QVERIFY(speedUp);
    speedUp->setTimeFactor(2.0);
    QCOMPARE(speedUp->timeFactor(), 2.0);

    // AudioComment: text, stable id and generated audio file.
    auto* audio= qobject_cast<AudioCommentEvent*>(comment);
    QVERIFY(audio);
    QVERIFY(!audio->id().isEmpty());
    audio->setText("Hello");
    QCOMPARE(audio->text(), QString("Hello"));
    audio->setAudioFile("/tmp/a/file_00.wav");
    QCOMPARE(audio->audioFile(), QString("/tmp/a/file_00.wav"));

    // Remove in the middle: the other events keep their identity.
    model.removeEffect(1);
    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.effectAt(0), cut);
    QCOMPARE(model.effectAt(1), comment);

    model.clear();
    QCOMPARE(model.rowCount(), 0);
}

void tst_tutorial::test_effectDataForwarding()
{
    VideoEffectModel model;
    model.addEffect(VideoTimedEvent::EventType::AUDIOCOMMENT, 5000, 8000);
    model.addEffect(VideoTimedEvent::EventType::SPEEDUPEVENT, 2000, 4000);

    auto* audio= qobject_cast<AudioCommentEvent*>(model.effectAt(0));
    auto* speed= qobject_cast<SpeedUpEvent*>(model.effectAt(1));
    QVERIFY(audio && speed);

    // Property changes on the events must be forwarded to dataChanged() so the
    // timeline tokens resize and the auto-save listeners fire.
    QSignalSpy spy(&model, &QAbstractItemModel::dataChanged);
    audio->setEnd(9000);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).value<QModelIndex>().row(), 0);

    audio->setText("Changed text");
    QCOMPARE(spy.count(), 1);
    spy.takeFirst();

    speed->setTimeFactor(0.5);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).value<QModelIndex>().row(), 1);

    // begin/end also forward.
    audio->setBegin(100);
    QCOMPARE(spy.count(), 1);
}

void tst_tutorial::test_model2JsonWithEffects()
{
    ClipModel clips;
    auto* info= new ClipInfo();
    info->addRange(0, 5000);
    clips.reset({info});

    VideoEffectModel effects;
    effects.addEffect(VideoTimedEvent::EventType::SPEEDUPEVENT, 1000, 2000);
    qobject_cast<SpeedUpEvent*>(effects.effectAt(0))->setTimeFactor(3.5);
    effects.addEffect(VideoTimedEvent::EventType::AUDIOCOMMENT, 3000, 6000);
    qobject_cast<AudioCommentEvent*>(effects.effectAt(1))->setText("Hi");

    const QByteArray json= Utils::model2Json("/videos/v.mp4", &clips, "tutorial", &effects);
    QJsonDocument doc= QJsonDocument::fromJson(json);
    QVERIFY(doc.isObject());
    auto obj= doc.object();
    QCOMPARE(obj["video"].toString(), QString("/videos/v.mp4"));
    QCOMPARE(obj["projectType"].toString(), QString("tutorial"));
    QCOMPARE(obj["clips"].toArray().size(), 1);

    auto eff= obj["effects"].toArray();
    QCOMPARE(eff.size(), 2);
    auto e0= eff[0].toObject();
    QCOMPARE(e0["type"].toInt(), int(VideoTimedEvent::EventType::SPEEDUPEVENT));
    QCOMPARE(e0["begin"].toDouble(), 1000.0);
    QCOMPARE(e0["end"].toDouble(), 2000.0);
    QCOMPARE(e0["timeFactor"].toDouble(), 3.5);
    auto e1= eff[1].toObject();
    QCOMPARE(e1["type"].toInt(), int(VideoTimedEvent::EventType::AUDIOCOMMENT));
    QCOMPARE(e1["text"].toString(), QString("Hi"));
    QVERIFY(!e1["id"].toString().isEmpty());

    // Without an effects model, no "effects" key is written.
    const QByteArray plain= Utils::model2Json("/videos/v.mp4", &clips);
    QVERIFY(!QJsonDocument::fromJson(plain).object().contains("effects"));
}

void tst_tutorial::test_piperCommandBuilding()
{
    TtsEngine engine;
    engine.setScriptPath("/opt/piper/gen_audio_files.py");
    engine.setPythonPath("/opt/piper/.venv/bin/python3");

    const QString dest= QDir::tempPath() + "/clipselector_piper_test";
    QDir().mkpath(dest);

    const QStringList args= engine.buildCommand("Hello world", dest, "fr");

    // The command must be: <script> -t <texts-file> -d <dest> -l <lang>
    QCOMPARE(args.size(), 7);
    QCOMPARE(args[0], QString("/opt/piper/gen_audio_files.py"));
    QCOMPARE(args[1], QString("-t"));
    QCOMPARE(args[3], QString("-d"));
    QCOMPARE(args[4], dest);
    QCOMPARE(args[5], QString("-l"));
    QCOMPARE(args[6], QString("fr"));

    // The script reads the texts from a JSON *file* (an array of strings),
    // written next to the generated audio.
    const QString textFile= QDir(dest).filePath("texts.json");
    QCOMPARE(args[2], textFile);
    QVERIFY(QFileInfo::exists(textFile));
    QFile file(textFile);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QJsonArray texts= QJsonDocument::fromJson(file.readAll()).array();
    QCOMPARE(texts.size(), 1);
    QCOMPARE(texts[0].toString(), QString("Hello world"));

    // The python interpreter is the program (not part of the args list).
    QCOMPARE(engine.pythonPath(), QString("/opt/piper/.venv/bin/python3"));
}

void tst_tutorial::test_ttsPathDerivation()
{
    // The script path is the single source of truth: the voice (language)
    // models are looked up in the script's directory and the python
    // interpreter is the .venv in that directory.
    TtsEngine engine;
    engine.setScriptPath("/some/where/v1.10/gen_audio_files.py");
    QCOMPARE(engine.modelDirectory(), QString("/some/where/v1.10"));
    QCOMPARE(engine.pythonPath(), QString("/some/where/v1.10/.venv/bin/python3"));

    // Explicit overrides still win.
    engine.setPythonPath("/usr/bin/python3");
    engine.setModelDirectory("/opt/models");
    QCOMPARE(engine.pythonPath(), QString("/usr/bin/python3"));
    QCOMPARE(engine.modelDirectory(), QString("/opt/models"));

    // Changing the script again re-derives both.
    engine.setScriptPath("/other/dir/gen_audio_files.py");
    QCOMPARE(engine.modelDirectory(), QString("/other/dir"));
    QCOMPARE(engine.pythonPath(), QString("/other/dir/.venv/bin/python3"));

    // The default constructor points at the v1.10 Piper environment.
    TtsEngine def;
    QVERIFY(def.scriptPath().endsWith("/v1.10/gen_audio_files.py"));
    QCOMPARE(def.modelDirectory(),
             QFileInfo(def.scriptPath()).absolutePath());
    QCOMPARE(def.pythonPath(),
             QDir(QFileInfo(def.scriptPath()).absolutePath()).filePath(".venv/bin/python3"));
}

void tst_tutorial::test_ttsFailureDetected()
{
    // Point the engine at a non-existent interpreter: generation must fail and
    // be reported (exit code / error), never silently succeed.
    TtsEngine engine;
    engine.setPythonPath("/nonexistent/python3");
    engine.setScriptPath("/nonexistent/gen_audio_files.py");
    engine.setModelDirectory("/nonexistent/models");

    QString details;
    QCOMPARE(engine.isAvailable(&details), false);
    QVERIFY(!details.isEmpty());

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    auto result= engine.generateSync("some text", dir.path(), "fr");
    QCOMPARE(result.ok, false);
    QVERIFY(!result.error.isEmpty());
}

void tst_tutorial::test_settingsPersistence()
{
    TutorialController ctrl;
    ctrl.setLanguage("fr");
    ctrl.setVoice("fr_FR-mls-medium");
    ctrl.writeSettings();

    TutorialController ctrl2;
    ctrl2.loadSettings();
    QCOMPARE(ctrl2.language(), QString("fr"));
    QCOMPARE(ctrl2.voice(), QString("fr_FR-mls-medium"));
}

void tst_tutorial::test_srtValid()
{
    // Entry.endMs carries an ABSOLUTE end time (the controller sets
    // endMs = startMs + audioDurationMs).
    QList<SubtitleGenerator::Entry> entries;
    entries.append({"First cue", 0, 2000});        // 00:00 -> 00:02
    entries.append({"Second cue", 5000, 8000});    // 00:05 -> 00:08
    entries.append({"Third cue", 65000, 68000});   // 01:05 -> 01:08

    const QByteArray srt= SubtitleGenerator::generateSrt(entries);
    const auto cues= parseSrt(srt);

    QCOMPARE(cues.size(), 3);
    // Sequence numbers must be 1..N.
    QCOMPARE(cues[0].seq, 1);
    QCOMPARE(cues[1].seq, 2);
    QCOMPARE(cues[2].seq, 3);
    // Timestamps: end = start + duration.
    QCOMPARE(cues[1].startMs, 5000);
    QCOMPARE(cues[1].endMs, 8000);
    QCOMPARE(cues[2].startMs, 65000);
    QCOMPARE(cues[2].endMs, 68000);
    // Text preserved.
    QCOMPARE(cues[0].text, QString("First cue"));
    QCOMPARE(cues[1].text, QString("Second cue"));
    QCOMPARE(cues[2].text, QString("Third cue"));

    // Format spot-checks.
    QCOMPARE(SubtitleGenerator::formatTimestamp(0), QString("00:00:00,000"));
    QCOMPARE(SubtitleGenerator::formatTimestamp(65000), QString("00:01:05,000"));
    QCOMPARE(SubtitleGenerator::formatTimestamp(36610123), QString("10:10:10,123"));
}

void tst_tutorial::test_srtOverlapping()
{
    // Two overlapping cues (absolute start/end times): both must appear,
    // sorted by start. Entry.endMs is an absolute end, not a duration.
    QList<SubtitleGenerator::Entry> entries;
    entries.append({"B starts first", 1000, 4000});        // 00:01 -> 00:04
    entries.append({"A starts later but overlaps", 2000, 6000}); // 00:02 -> 00:06

    const QByteArray srt= SubtitleGenerator::generateSrt(entries);
    const auto cues= parseSrt(srt);

    QCOMPARE(cues.size(), 2);
    QCOMPARE(cues[0].text, QString("B starts first"));
    QCOMPARE(cues[1].text, QString("A starts later but overlaps"));
    QCOMPARE(cues[0].startMs, 1000);
    QCOMPARE(cues[1].startMs, 2000);
    QCOMPARE(cues[0].endMs, 4000);
    QCOMPARE(cues[1].endMs, 6000);
}

void tst_tutorial::test_srtFallbackDuration()
{
    // A cue with zero/negative duration must get a positive fallback.
    QList<SubtitleGenerator::Entry> entries;
    entries.append({"No duration", 1000, 0}); // duration 0 -> fallback

    QStringList problems;
    const QByteArray srt= SubtitleGenerator::generateSrt(entries, &problems);
    const auto cues= parseSrt(srt);

    QCOMPARE(cues.size(), 1);
    QVERIFY(cues[0].endMs > cues[0].startMs);
    QCOMPARE(cues[0].endMs - cues[0].startMs, SubtitleGenerator::kFallbackDurationMs);
    QVERIFY(!problems.isEmpty());
}

void tst_tutorial::test_audioFilterPlacement()
{
    QList<VideoRenderer::CommentAudio> audios;
    VideoRenderer::CommentAudio a1;
    a1.audioFile= "/tmp/a1.wav";
    a1.startMs= 1000;
    audios.append(a1);
    VideoRenderer::CommentAudio a2;
    a2.audioFile= "/tmp/a2.wav";
    a2.startMs= 5000;
    audios.append(a2);

    QString mixedLabel;
    const QString filter= VideoRenderer::buildAudioFilter(audios, true, 1.0, &mixedLabel);

    // Each clip must be delayed to its configured timestamp.
    QVERIFY(filter.contains("adelay=1000|0"));
    QVERIFY(filter.contains("adelay=5000|0"));
    // Inputs are [0:a] (source) plus the two narration inputs.
    QVERIFY(filter.contains("[0:a]"));
    QVERIFY(filter.contains("[1:a]"));
    QVERIFY(filter.contains("[2:a]"));
    // amix over 3 inputs, keeping the first (source) duration.
    QVERIFY(filter.contains("amix=inputs=3"));
    QVERIFY(filter.contains("duration=first"));
    QCOMPARE(mixedLabel, QString("[mixed]"));
}

void tst_tutorial::test_originalAudioPreserved()
{
    QList<VideoRenderer::CommentAudio> audios;
    VideoRenderer::CommentAudio a1;
    a1.audioFile= "/tmp/a1.wav";
    a1.startMs= 1000;
    audios.append(a1);

    QString withLabel, withoutLabel;
    const QString with= VideoRenderer::buildAudioFilter(audios, true, 1.0, &withLabel);
    const QString without= VideoRenderer::buildAudioFilter(audios, false, 1.0, &withoutLabel);

    // When keeping original audio, the source audio [0:a] is the mix base.
    QVERIFY(with.contains("[0:a]"));
    // When original audio is disabled, silence is synthesized instead.
    QVERIFY(!without.contains("[0:a]"));
    QVERIFY(without.contains("anullsrc"));
}

void tst_tutorial::test_rendererMissingInputs()
{
    // A real, existing file to act as a "present" source, so we can exercise
    // the narration-missing and empty-list branches independently.
    QTemporaryFile realSource;
    QVERIFY(realSource.open());
    const QString realSourcePath= realSource.fileName();
    realSource.close();

    VideoRenderer renderer;
    VideoRenderer::RenderOptions opts;
    opts.outputVideo= "/nonexistent/out.mp4";

    QString error;

    // Missing source video must fail with a useful message.
    {
        opts.sourceVideo= "/nonexistent/video.mp4";
        QList<VideoRenderer::CommentAudio> audios;
        VideoRenderer::CommentAudio a;
        a.audioFile= realSourcePath;
        a.startMs= 0;
        audios.append(a);
        QCOMPARE(renderer.renderSync(opts, audios, &error), false);
        QVERIFY(error.contains("Source video not found"));
    }

    // Missing narration audio must also fail with a useful message.
    {
        opts.sourceVideo= realSourcePath;
        QList<VideoRenderer::CommentAudio> audios;
        VideoRenderer::CommentAudio a;
        a.audioFile= "/nonexistent/a.wav";
        a.startMs= 0;
        audios.append(a);
        QCOMPARE(renderer.renderSync(opts, audios, &error), false);
        QVERIFY(error.contains("Narration audio not found"));
    }

    // Empty narration list must also fail.
    {
        opts.sourceVideo= realSourcePath;
        QCOMPARE(renderer.renderSync(opts, {}, &error), false);
        QVERIFY(error.contains("No narration audio"));
    }
}

void tst_tutorial::test_cancelRender()
{
    // renderAsync with a missing source must report failure without hanging.
    TutorialController ctrl;
    ctrl.setVideoFilePath("/nonexistent/video.mp4");

    QSignalSpy spy(&ctrl, &TutorialController::workflowFinished);
    ctrl.renderVideo(true, true);
    QCOMPARE(spy.count(), 1);
    auto args= spy.takeFirst();
    QCOMPARE(args[0].toBool(), false);
}

void tst_tutorial::test_projectRoundtrip()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString projectPath= dir.path() + "/proj.json";

    QString commentId;
    {
        TutorialController ctrl;
        // No real Piper in the tests: point the engine at non-existent paths so
        // setting a comment text never spawns a generation process.
        ctrl.ttsEngine()->setScriptPath("/nonexistent/gen_audio_files.py");
        ctrl.ttsEngine()->setPythonPath("/nonexistent/python3");
        ctrl.ttsEngine()->setModelDirectory("/nonexistent/models");

        ctrl.setProjectFilePath(projectPath);
        ctrl.setVideoFilePath("/videos/source.mp4");
        ctrl.setLanguage("fr");
        ctrl.setVoice("fr_FR-mls-medium");

        // Build the timeline: a comment, a speed-up and a cut.
        ctrl.addCommentAtCurrentPosition(1000, 4000);
        auto* c1= qobject_cast<AudioCommentEvent*>(ctrl.effects()->effectAt(0));
        QVERIFY(c1);
        commentId= c1->id();
        c1->setText("Intro");
        c1->setAudioFile("/videos/source.json.tutorial/audio/" + commentId + "/file_00.wav");

        ctrl.addSpeedEffect(90000, 95000);
        auto* s1= qobject_cast<SpeedUpEvent*>(ctrl.effects()->effectAt(1));
        QVERIFY(s1);
        s1->setTimeFactor(2.0);

        ctrl.addCutEffect(120000, 120000);
        QCOMPARE(ctrl.effects()->rowCount(), 3);

        ctrl.saveProject(projectPath);
        QVERIFY(QFileInfo::exists(projectPath));
    }

    {
        TutorialController ctrl;
        ctrl.loadProject(projectPath);

        QCOMPARE(ctrl.videoFilePath(), QString("/videos/source.mp4"));
        QCOMPARE(ctrl.effects()->rowCount(), 3);

        auto* c1= qobject_cast<AudioCommentEvent*>(ctrl.effects()->effectAt(0));
        QVERIFY(c1);
        QCOMPARE(c1->text(), QString("Intro"));
        QCOMPARE(c1->begin(), qint64(1000));
        QCOMPARE(c1->end(), qint64(4000));
        QCOMPARE(c1->id(), commentId);
        QCOMPARE(c1->audioFile(), QString("/videos/source.json.tutorial/audio/" + commentId + "/file_00.wav"));

        auto* s1= qobject_cast<SpeedUpEvent*>(ctrl.effects()->effectAt(1));
        QVERIFY(s1);
        QCOMPARE(s1->timeFactor(), 2.0);
        QCOMPARE(s1->begin(), qint64(90000));
        QCOMPARE(s1->end(), qint64(95000));

        auto* cut= ctrl.effects()->effectAt(2);
        QVERIFY(cut);
        QCOMPARE(cut->type(), VideoTimedEvent::EventType::CUTEVENT);
        QCOMPARE(cut->begin(), qint64(120000));
    }
}

void tst_tutorial::test_effectPlayback()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    // Generate a 10s test video and a 1s comment audio with ffmpeg.
    const QString videoPath= dir.path() + "/test.mp4";
    const QString wavPath= dir.path() + "/comment.wav";
    {
        QProcess ff;
        ff.start("ffmpeg", {"-y", "-f", "lavfi", "-i", "testsrc=duration=10:size=320x240:rate=25",
                            "-f", "lavfi", "-i", "sine=frequency=440:duration=10",
                            "-c:v", "libx264", "-preset", "ultrafast", "-pix_fmt", "yuv420p",
                            "-g", "25", "-shortest", videoPath});
        QVERIFY(ff.waitForFinished(60000));
        QProcess ff2;
        ff2.start("ffmpeg", {"-y", "-f", "lavfi", "-i", "sine=frequency=880:duration=1", wavPath});
        QVERIFY(ff2.waitForFinished(60000));
    }
    QVERIFY(QFileInfo::exists(videoPath));
    QVERIFY(QFileInfo::exists(wavPath));

    VideoEffectModel effects;
    effects.addEffect(VideoTimedEvent::EventType::CUTEVENT, 1000, 2000);
    effects.addEffect(VideoTimedEvent::EventType::SPEEDUPEVENT, 2000, 8000);
    auto* speed= qobject_cast<SpeedUpEvent*>(effects.effectAt(1));
    QVERIFY(speed);
    speed->setTimeFactor(2.0);
    effects.addEffect(VideoTimedEvent::EventType::AUDIOCOMMENT, 5000, 6000);
    auto* comment= qobject_cast<AudioCommentEvent*>(effects.effectAt(2));
    QVERIFY(comment);
    comment->setAudioFile(wavPath);

    QMediaPlayer player;
    player.setSource(QUrl::fromLocalFile(videoPath));

    EffectPlayback playback(&effects);
    playback.setPlayer(&player);

    // Wait until the media is actually loaded (no multimedia backend -> skip).
    bool ready= false;
    for(int i= 0; i < 100 && !ready; ++i)
    {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        ready= player.mediaStatus() == QMediaPlayer::LoadedMedia
               || player.mediaStatus() == QMediaPlayer::BufferedMedia;
    }
    if(!ready)
        QSKIP("Multimedia backend could not load the test video", Continue);

    // Record (position, playbackRate) pairs as the playback advances.
    struct Sample
    {
        qint64 pos;
        float rate;
    };
    QVector<Sample> samples;
    QObject::connect(&player, &QMediaPlayer::positionChanged, &player,
                     [&player, &samples](qint64 pos)
                     { samples.append({pos, static_cast<float>(player.playbackRate())}); });
    // playingChanged() carries no argument: query the state when it fires.
    bool soundPlayed= false;
    QObject::connect(playback.commentSound(), &QSoundEffect::playingChanged, &player,
                     [&playback, &soundPlayed]()
                     { if(playback.commentSound()->isPlaying()) soundPlayed= true; });

    player.play();

    // 10s video, 6s of it at 2x speed => a bit less than 7s of wall time.
    QTRY_VERIFY_WITH_TIMEOUT(player.position() > 8000, 20000);

    // --- Cut [1000, 2000): once the playhead reaches the cut end it must
    // never go back inside the cut range.
    int firstBeyondCut= -1;
    for(int i= 0; i < samples.size(); ++i)
    {
        if(samples[i].pos >= 2000)
        {
            firstBeyondCut= i;
            break;
        }
    }
    QVERIFY(firstBeyondCut >= 0);
    // The backend may deliver one stale position update from before the seek
    // right after the jump; tolerate a couple of transient samples in the cut
    // range, but the playhead must not dwell there.
    int staleInCut= 0;
    for(int i= firstBeyondCut; i < samples.size(); ++i)
    {
        if(samples[i].pos < 2000)
            ++staleInCut;
    }
    QVERIFY2(staleInCut <= 2, qPrintable(QString("%1 samples stayed inside the cut range").arg(staleInCut)));

    // --- Speed [2000, 8000) at 2x: the rate must be applied inside the
    // segment and restored after it.
    bool sawSpeedRate= false;
    for(const auto& s : samples)
    {
        if(s.pos >= 2000 && s.pos < 8000 && s.rate == 2.0f)
            sawSpeedRate= true;
    }
    QVERIFY(sawSpeedRate);
    bool restoredAfterSpeed= false;
    for(const auto& s : samples)
    {
        if(s.pos >= 8000 && s.rate == 1.0f)
            restoredAfterSpeed= true;
    }
    QVERIFY(restoredAfterSpeed);

    // --- Comment @5000: its audio must have started.
    QVERIFY(soundPlayed);
}

void tst_tutorial::test_effectModelRoles()
{
    VideoEffectModel model;
    model.addEffect(VideoTimedEvent::EventType::CUTEVENT, 1000, 2000);
    model.addEffect(VideoTimedEvent::EventType::SPEEDUPEVENT, 3000, 5000);
    model.addEffect(VideoTimedEvent::EventType::AUDIOCOMMENT, 6000, 9000);
    auto* audio= qobject_cast<AudioCommentEvent*>(model.effectAt(2));
    QVERIFY(audio);
    audio->setAudioFile("/tmp/x/file_00.wav");

    QCOMPARE(model.data(model.index(0), VideoEffectModel::TypeNameRole).toString(), QString("Cut"));
    QCOMPARE(model.data(model.index(1), VideoEffectModel::TypeNameRole).toString(), QString("Speed up"));
    QCOMPARE(model.data(model.index(2), VideoEffectModel::TypeNameRole).toString(), QString("Audio comment"));
    QCOMPARE(model.data(model.index(0), VideoEffectModel::AudioFileRole).toString(), QString());
    QCOMPARE(model.data(model.index(2), VideoEffectModel::AudioFileRole).toString(), QString("/tmp/x/file_00.wav"));

    // The QML side binds to these role names.
    const auto names= model.roleNames();
    QCOMPARE(names.value(VideoEffectModel::TypeNameRole), QByteArray("typeName"));
    QCOMPARE(names.value(VideoEffectModel::AudioFileRole), QByteArray("audioFile"));
}

void tst_tutorial::test_audioFileManagement()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString wavPath= dir.path() + "/file_00.wav";
    {
        QFile f(wavPath);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("RIFFfake");
    }

    TutorialController ctrl;
    ctrl.addCommentWithText(1000, 4000, "Hello");
    auto* comment= qobject_cast<AudioCommentEvent*>(ctrl.effects()->effectAt(0));
    QVERIFY(comment);
    comment->setAudioFile(wavPath);

    // Removing the audio deletes the file and clears the reference, while the
    // comment itself (text, position) is kept.
    ctrl.removeAudio(comment->id());
    QVERIFY(!QFileInfo::exists(wavPath));
    QCOMPARE(comment->audioFile(), QString());
    QCOMPARE(ctrl.effects()->rowCount(), 1);
    QCOMPARE(comment->text(), QString("Hello"));

    // Regenerating without Piper (isolated settings in initTestCase) must fail
    // gracefully with a status message.
    QString details;
    QVERIFY(!ctrl.ttsEngine()->isAvailable(&details));
    ctrl.regenerateAudio(comment->id());
    QVERIFY(ctrl.statusMessage().contains("Piper is not available"));

    // Unknown ids are ignored without crashing.
    ctrl.removeAudio("does-not-exist");
    ctrl.regenerateAudio("does-not-exist");
}

namespace
{
// This stripped Qt build cannot deliver synthetic mouse events to a
// QQuickItem (hand-made QMouseEvents carry no window, QML's
// Item.mousePress() is absent, and the QWindowSystemInterface header is
// missing). Expose the protected handlers so a test can run the exact code
// a real drag would run: select on press, move on a Middle drag, which must
// emit timeChanged() so the delegate pushes the new x/width back to the model.
class TestableToken : public TimeLineEffectToken
{
public:
    using TimeLineEffectToken::TimeLineEffectToken;
    void testMousePress(QMouseEvent* e) { mousePressEvent(e); }
    void testMouseMove(QMouseEvent* e) { mouseMoveEvent(e); }
    void testMouseRelease(QMouseEvent* e) { mouseReleaseEvent(e); }
};
} // namespace

void tst_tutorial::test_tokenMoveUpdatesCommentPosition()
{
    // Moving a token on the timeline must push the new begin/end back to the
    // model, so the effects (e.g. the comment audio start position) follow
    // the moved token instead of staying at the creation position.
    VideoEffectModel effects;
    effects.addEffect(VideoTimedEvent::EventType::AUDIOCOMMENT, 200000, 300000);
    auto* comment= qobject_cast<AudioCommentEvent*>(effects.effectAt(0));
    QVERIFY(comment);

    // Mirror the QML timeline delegate: 10 px per second, and the
    // timeChanged handler pushes x/width back to the model row.
    TestableToken token;
    token.setTimedEvent(comment);
    token.setSecondInPixel(10.0);
    token.setX(2000);
    token.setWidth(1000);
    token.setHeight(28);
    QObject::connect(&token, &TimeLineTokenBase::timeChanged, &token,
                     [&token, comment]
                     {
                         comment->setBegin(qint64(token.x() * 1000 / token.secondInPixel()));
                         comment->setEnd(qint64((token.x() + token.width()) * 1000 / token.secondInPixel()));
                     });

    // First click selects the token (well away from the resize handles).
    QMouseEvent press1(QEvent::MouseButtonPress, QPointF(100, 14), QPointF(100, 14), QPointF(100, 14),
                       Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    token.testMousePress(&press1);
    QMouseEvent rel1(QEvent::MouseButtonRelease, QPointF(100, 14), QPointF(100, 14), QPointF(100, 14),
                     Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    token.testMouseRelease(&rel1);
    QCOMPARE(token.selected(), true);

    // Second click starts a Middle drag, then drag +100 px to the right.
    QMouseEvent press2(QEvent::MouseButtonPress, QPointF(100, 14), QPointF(100, 14), QPointF(100, 14),
                       Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    token.testMousePress(&press2);
    QMouseEvent move(QEvent::MouseMove, QPointF(200, 14), QPointF(200, 14), QPointF(200, 14),
                     Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    token.testMouseMove(&move);
    QMouseEvent rel2(QEvent::MouseButtonRelease, QPointF(200, 14), QPointF(200, 14), QPointF(200, 14),
                     Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    token.testMouseRelease(&rel2);

    // The model must have followed the token.
    QCOMPARE(token.x(), qreal(2100));
    QCOMPARE(comment->begin(), qint64(210000));
    QCOMPARE(comment->end(), qint64(310000));
}

// --- SRT parsing helpers ---

qint64 tst_tutorial::parseSrtTimestamp(const QString& ts)
{
    // "HH:MM:SS,mmm" -> milliseconds.
    const auto parts= ts.split(':');
    if(parts.size() != 3)
        return -1;
    const int h= parts[0].toInt();
    const int m= parts[1].toInt();
    const auto secMs= parts[2].split(',');
    if(secMs.size() != 2)
        return -1;
    const int s= secMs[0].toInt();
    const int ms= secMs[1].toInt();
    return qint64(((h * 60 + m) * 60 + s)) * 1000 + ms;
}

QList<tst_tutorial::SrtCue> tst_tutorial::parseSrt(const QByteArray& data)
{
    QList<SrtCue> cues;
    const auto blocks= QString::fromUtf8(data).split("\n\n");
    for(const auto& block : blocks)
    {
        if(block.trimmed().isEmpty())
            continue;
        auto lines= block.split('\n');
        if(lines.size() < 3)
            continue;

        SrtCue cue;
        cue.seq= lines[0].trimmed().toInt();
        const auto times= lines[1].split(" --> ");
        if(times.size() != 2)
            continue;
        cue.startMs= parseSrtTimestamp(times[0].trimmed());
        cue.endMs= parseSrtTimestamp(times[1].trimmed());
        if(cue.startMs < 0 || cue.endMs < 0)
            continue;
        QStringList text;
        for(int i= 2; i < lines.size(); ++i)
            text.append(lines[i]);
        cue.text= text.join("\n");
        cues.append(cue);
    }
    return cues;
}

QTEST_MAIN(tst_tutorial)
#include "tst_tutorial.moc"