#ifndef VIDEORENDERER_H
#define VIDEORENDERER_H

#include <QObject>
#include <QProcess>
#include <QStringList>

// Renders the final TUTORIAL video by mixing generated narration over the
// source video (and optionally embedding subtitles) using FFmpeg.
//
// The ffmpeg command construction is exposed as pure functions so the audio
// placement / filter graph can be unit-tested without running FFmpeg.
class VideoRenderer : public QObject
{
    Q_OBJECT
public:
    struct CommentAudio
    {
        QString audioFile; // absolute path to the narration wav
        qint64 startMs;    // position on the source video timeline
    };

    struct RenderOptions
    {
        QString sourceVideo;   // absolute path to the source video
        QString outputVideo;   // absolute path for the final video
        QString srtFile;       // absolute path to the SRT (for embedding)
        bool keepOriginalAudio{true};   // mix source audio under narration
        bool embedSubtitles{true};      // burn subtitles into the video
        double narrationGain{1.0};      // linear gain applied to each narration
        QString ffmpegPath{"ffmpeg"};
    };

    explicit VideoRenderer(QObject* parent= nullptr);

    // True when the ffmpeg executable is present.
    static bool isAvailable(const QString& ffmpegPath= "ffmpeg",
                            QString* error= nullptr);

    // Build the narration audio filter chain:
    //   [i:a]adelay=<ms>[n]; ... [n][m]amix=...
    // Returns the filter_complex string and the mixed audio label used as the
    // final audio input. Pure (no process).
    static QString buildAudioFilter(const QList<CommentAudio>& audios,
                                    bool keepOriginalAudio,
                                    double gain,
                                    QString* mixedLabel= nullptr);

    // Build the complete ffmpeg argument list for the render.
    static QStringList buildCommand(const RenderOptions& opts,
                                    const QList<CommentAudio>& audios);

    // Run the render. Non-blocking; emits renderFinished()/renderProgress().
    void renderAsync(const RenderOptions& opts, const QList<CommentAudio>& audios);

    // Synchronous render (used by tests / rendering preparation).
    bool renderSync(const RenderOptions& opts, const QList<CommentAudio>& audios,
                    QString* error= nullptr);

    // Cancel the in-flight render (safe: removes the temporary output).
    void cancel();

    // True while a render is running.
    bool isRunning() const;

signals:
    void renderStarted();
    void renderProgress(qint64 positionMs, qint64 totalMs);
    void renderFinished(bool success, const QString& message);

private:
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);

    QProcess* m_process{nullptr};
    QString m_tempOutput;
    QString m_finalOutput;
    bool m_cancelled{false};
};

#endif // VIDEORENDERER_H
