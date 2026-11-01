#ifndef TUTORIALCONTROLLER_H
#define TUTORIALCONTROLLER_H

#include <QObject>
#include <QPointer>
#include <QQueue>
#include <QUrl>

#include "ttsengine.h"
#include "videoeffect.h"
#include "videorenderer.h"

class QSettings;

// Orchestrates the TUTORIAL workflow: comment editing, Piper audio generation,
// SRT generation and final video rendering. Exposed to QML as a singleton.
class TutorialController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    // clang-format off
    Q_PROPERTY(VideoEffectModel* effects READ effects CONSTANT)
    Q_PROPERTY(QString videoFilePath READ videoFilePath WRITE setVideoFilePath NOTIFY videoFilePathChanged FINAL)
    Q_PROPERTY(QString projectFilePath READ projectFilePath WRITE setProjectFilePath NOTIFY projectFilePathChanged FINAL)
    Q_PROPERTY(int stage READ stage NOTIFY stageChanged FINAL)
    Q_PROPERTY(int current READ current NOTIFY progressChanged FINAL)
    Q_PROPERTY(int total READ total NOTIFY progressChanged FINAL)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged FINAL)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged FINAL)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged FINAL)
    Q_PROPERTY(QString voice READ voice WRITE setVoice NOTIFY voiceChanged FINAL)
    Q_PROPERTY(QStringList languages READ languages NOTIFY languagesChanged FINAL)
    Q_PROPERTY(QStringList voices READ voices NOTIFY voicesChanged FINAL)
    Q_PROPERTY(QString srtPath READ srtPath NOTIFY srtPathChanged FINAL)
    Q_PROPERTY(QString outputVideoPath READ outputVideoPath NOTIFY outputVideoPathChanged FINAL)
    Q_PROPERTY(TtsEngine* ttsEngine READ ttsEngine CONSTANT)
    Q_PROPERTY(VideoRenderer* renderer READ renderer CONSTANT)
    Q_PROPERTY(qint64 currentPositionMs READ currentPositionMs WRITE setCurrentPositionMs NOTIFY currentPositionMsChanged FINAL)
    // clang-format on
public:
    enum Stage
    {
        Idle= 0,
        GeneratingAudio,
        GeneratingSubtitles,
        Rendering,
        Done
    };
    Q_ENUM(Stage)

    explicit TutorialController(QObject* parent= nullptr);
    ~TutorialController();

    VideoEffectModel* effects() const;
    TtsEngine* ttsEngine() const;
    VideoRenderer* renderer() const;

    QString videoFilePath() const;
    void setVideoFilePath(const QString& path);

    QString projectFilePath() const;
    void setProjectFilePath(const QString& path);

    int stage() const;
    int current() const;
    int total() const;
    QString statusMessage() const;
    bool busy() const;

    // --- TTS settings ---
    QString language() const;
    void setLanguage(const QString& lang);
    QString voice() const;
    void setVoice(const QString& voice);
    QStringList languages() const;
    QStringList voices() const;

    QString srtPath() const;
    QString outputVideoPath() const;

    // Project working directory holding generated assets (audio/, subtitles.srt,
    // tutorial_output.mp4). Exposed so the UI can show where files are stored.
    QString workingDirectory() const;

    qint64 currentPositionMs() const;
    void setCurrentPositionMs(qint64 positionMs);

    void removeEffect(int id);

    // --- Workflow ---
    // Generate audio for every comment that is pending/outdated/errored.
    void generateAudio();
    // Generate (and write) the SRT file for the current comments.
    void generateSubtitle();
    // Render the final video.
    void renderVideo(bool keepOriginalAudio, bool embedSubtitles);

    // --- Project persistence ---
    void saveProject(const QString& path);
    void loadProject(const QString& path);

    // Persist/restore TTS settings with the application's QSettings.
    void writeSettings() const;
    void loadSettings();

public slots:
    void addCommentAtCurrentPosition(qint64 start, qint64 end);
    // Add an audio comment and set its text (which triggers TTS generation).
    void addCommentWithText(qint64 start, qint64 end, const QString& text);
    void addSpeedEffect(qint64 start, qint64 end);
    void addCutEffect(qint64 start, qint64 end);

    // Regenerate the Piper audio of a single comment (by its stable id):
    // the old file is deleted and the text is sent through the TTS queue.
    void regenerateAudio(const QString& commentId);
    // Delete the generated audio file of a single comment (by its stable id)
    // and clear the reference. The comment itself (text, position) is kept.
    void removeAudio(const QString& commentId);

signals:
    void videoFilePathChanged();
    void projectFilePathChanged();
    void stageChanged();
    void progressChanged();
    void statusMessageChanged();
    void busyChanged();
    void languageChanged();
    void voiceChanged();
    void languagesChanged();
    void voicesChanged();
    void srtPathChanged();
    void outputVideoPathChanged();
    void currentPositionMsChanged();
    void workflowFinished(bool success, const QString& message);

private slots:
    void onGenerationFinished(const QString& text, const TtsEngine::GenerationResult& result);

private:
    void setStage(Stage stage);
    void setStatus(const QString& message);
    void setBusy(bool busy);
    // Pop the next AudioCommentEvent from the queue and start its TTS
    // generation (the TtsEngine is single-process, one generation at a time).
    void processNextInQueue();
    // Connect the textChanged signal of every AudioCommentEvent to the TTS
    // queue. Idempotent (UniqueConnection); re-run when new rows appear.
    void connectEvents();
    // Resolve the project-relative working directory for generated assets.
    QString audioDirFor(const QString& commentId) const;
    // Find an audio comment by its stable id (nullptr when absent).
    AudioCommentEvent* findComment(const QString& id) const;

    VideoEffectModel* m_effects;
    TtsEngine* m_tts;
    VideoRenderer* m_renderer;

    QString m_videoFilePath;
    QString m_projectFilePath;
    int m_stage{Idle};
    int m_current{0};
    int m_total{0};
    QString m_statusMessage;
    bool m_busy{false};

    QString m_language;
    QString m_voice;

    QString m_srtPath;
    QString m_outputVideoPath;
    qint64 m_currentPositionMs{0};

    // Queue of comments awaiting audio generation.
    QQueue<QPointer<AudioCommentEvent>> m_audioQueue;
    // The comment whose TTS generation is currently in flight.
    QPointer<AudioCommentEvent> m_generatingEvent;
};

#endif // TUTORIALCONTROLLER_H
