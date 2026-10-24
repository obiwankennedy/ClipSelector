#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QSignalSpy>
#include <QtTest>

#include "maincontroller.h"
#include "subtitlegenerator.h"
#include "tutorialcomment.h"
#include "ttsengine.h"
#include "tutorialcontroller.h"
#include "videorenderer.h"

class tst_tutorial : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void test_modeNames();
    void test_appModeDefaults();
    void test_commentCrud();
    void test_commentsSortedByTime();
    void test_staleInvalidation();
    void test_timestampChangeKeepsAudio();
    void test_piperCommandBuilding();
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
    // Use an isolated QSettings scope so tests never clobber real settings.
    qputenv("XDG_CONFIG_HOME", (QCoreApplication::applicationDirPath() + "/tst_settings").toUtf8());
}

void tst_tutorial::cleanupTestCase()
{
    qunsetenv("XDG_CONFIG_HOME");
}

void tst_tutorial::test_modeNames()
{
    const auto names= MainController::modeNames();
    QCOMPARE(names.size(), 2);
    QCOMPARE(names[0], QString::fromUtf8("MUSIC CLIP"));
    QCOMPARE(names[1], QString::fromUtf8("TUTORIAL"));
}

void tst_tutorial::test_appModeDefaults()
{
    MainController ctrl;
    QCOMPARE(ctrl.appMode(), int(MainController::MusicClip));
    ctrl.setAppMode(MainController::Tutorial);
    QCOMPARE(ctrl.appMode(), int(MainController::Tutorial));
}

void tst_tutorial::test_commentCrud()
{
    TutorialCommentModel model;
    QCOMPARE(model.rowCount(), 0);

    const QString id= model.addComment("Hello", 1000, "fr", "fr_FR-mls-medium");
    QCOMPARE(model.rowCount(), 1);

    auto* c= model.commentAt(0);
    QVERIFY(c != nullptr);
    QCOMPARE(c->id(), id);
    QCOMPARE(c->text(), QString("Hello"));
    QCOMPARE(c->startTimeMs(), 1000);
    QCOMPARE(c->language(), QString("fr"));
    QCOMPARE(c->voice(), QString("fr_FR-mls-medium"));

    // Edit text.
    c->setText("Changed");
    QCOMPARE(c->text(), QString("Changed"));

    // Move on the timeline.
    c->setStartTimeMs(5000);
    QCOMPARE(c->startTimeMs(), 5000);

    // Delete.
    model.removeComment(0);
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(model.indexOfId(id), -1);
}

void tst_tutorial::test_commentsSortedByTime()
{
    TutorialCommentModel model;
    model.addComment("third", 3000, "fr", "v");
    model.addComment("first", 1000, "fr", "v");
    model.addComment("second", 2000, "fr", "v");
    model.addComment("zero", 0, "fr", "v");

    auto sorted= model.sortedByTime();
    QCOMPARE(sorted.size(), 4);
    QCOMPARE(sorted[0]->text(), QString("zero"));
    QCOMPARE(sorted[1]->text(), QString("first"));
    QCOMPARE(sorted[2]->text(), QString("second"));
    QCOMPARE(sorted[3]->text(), QString("third"));

    // Insertion order is preserved by allComments().
    auto all= model.allComments();
    QCOMPARE(all[0]->text(), QString("third"));
}

void tst_tutorial::test_staleInvalidation()
{
    auto c= new TutorialComment;
    c->setText("original");
    c->setLanguage("fr");
    c->setVoice("fr_FR-mls-medium");
    c->markGenerated();
    QCOMPARE(c->status(), int(TutorialComment::Ready));
    QCOMPARE(c->isStale(), false);

    // Changing text invalidates.
    c->setText("edited");
    QCOMPARE(c->isStale(), true);
    QCOMPARE(c->status(), int(TutorialComment::Outdated));

    // Regenerating clears staleness.
    c->markGenerated();
    QCOMPARE(c->isStale(), false);

    // Changing language invalidates.
    c->setLanguage("en");
    QCOMPARE(c->isStale(), true);

    // Changing voice invalidates.
    c->setLanguage("fr");
    c->markGenerated();
    c->setVoice("other_voice");
    QCOMPARE(c->isStale(), true);
    delete c;
}

void tst_tutorial::test_timestampChangeKeepsAudio()
{
    auto c= new TutorialComment;
    c->setText("text");
    c->setLanguage("fr");
    c->setVoice("v");
    c->markGenerated();
    QCOMPARE(c->isStale(), false);

    // Moving on the timeline must NOT invalidate the audio.
    c->setStartTimeMs(12345);
    QCOMPARE(c->isStale(), false);
    QCOMPARE(c->status(), int(TutorialComment::Ready));
    delete c;
}

void tst_tutorial::test_piperCommandBuilding()
{
    TtsEngine engine;
    engine.setScriptPath("/opt/piper/gen_audio_files.py");
    engine.setPythonPath("/opt/piper/.venv/bin/python3");

    const QStringList args= engine.buildCommand("Hello world", "/tmp/out", "fr");

    // The command must be: <script> -t <json-array> -d <dest> -l <lang>
    QCOMPARE(args.size(), 7);
    QCOMPARE(args[0], QString("/opt/piper/gen_audio_files.py"));
    QCOMPARE(args[1], QString("-t"));
    QCOMPARE(args[3], QString("-d"));
    QCOMPARE(args[4], QString("/tmp/out"));
    QCOMPARE(args[5], QString("-l"));
    QCOMPARE(args[6], QString("fr"));

    // The text argument must be a JSON array containing the text verbatim.
    QJsonArray texts= QJsonDocument::fromJson(args[2].toUtf8()).array();
    QCOMPARE(texts.size(), 1);
    QCOMPARE(texts[0].toString(), QString("Hello world"));

    // The python interpreter is the program (not part of the args list).
    QCOMPARE(engine.pythonPath(), QString("/opt/piper/.venv/bin/python3"));
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

    {
        TutorialController ctrl;
        ctrl.setProjectFilePath(projectPath);
        ctrl.setVideoFilePath("/videos/source.mp4");
        ctrl.setLanguage("fr");
        ctrl.setVoice("fr_FR-mls-medium");

        // Add comments with known audio durations (no real generation needed).
        const QString c1= ctrl.comments()->addComment("Intro", 1000, "fr", "fr_FR-mls-medium");
        const QString c2= ctrl.comments()->addComment("Conclusion", 90000, "fr", "fr_FR-mls-medium");
        ctrl.comments()->commentAt(ctrl.comments()->indexOfId(c1))->setAudioDurationMs(3000);
        ctrl.comments()->commentAt(ctrl.comments()->indexOfId(c2))->setAudioDurationMs(4000);

        ctrl.saveProject(projectPath);
        QVERIFY(QFileInfo::exists(projectPath));
    }

    {
        TutorialController ctrl;
        ctrl.loadProject(projectPath);

        QCOMPARE(ctrl.videoFilePath(), QString("/videos/source.mp4"));
        QCOMPARE(ctrl.language(), QString("fr"));
        QCOMPARE(ctrl.voice(), QString("fr_FR-mls-medium"));
        QCOMPARE(ctrl.comments()->rowCount(), 2);

        const auto sorted= ctrl.comments()->sortedByTime();
        QCOMPARE(sorted[0]->text(), QString("Intro"));
        QCOMPARE(sorted[0]->startTimeMs(), 1000);
        QCOMPARE(sorted[0]->audioDurationMs(), 3000);
        QCOMPARE(sorted[1]->text(), QString("Conclusion"));
        QCOMPARE(sorted[1]->startTimeMs(), 90000);
        QCOMPARE(sorted[1]->audioDurationMs(), 4000);

        // The SRT can be regenerated from the saved metadata.
        ctrl.generateSubtitle();
        const QString srt= ctrl.srtPath();
        QVERIFY(QFileInfo::exists(srt));
        QFile f(srt);
        QVERIFY(f.open(QIODevice::ReadOnly));
        const auto data= f.readAll();
        const auto cues= parseSrt(data);
        QCOMPARE(cues.size(), 2);
        QCOMPARE(cues[0].text, QString("Intro"));
        QCOMPARE(cues[1].text, QString("Conclusion"));
    }
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