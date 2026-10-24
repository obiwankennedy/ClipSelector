#include "videorenderer.h"

#include "ttsengine.h"

#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QTemporaryFile>

VideoRenderer::VideoRenderer(QObject* parent)
    : QObject(parent)
{
}

bool VideoRenderer::isAvailable(const QString& ffmpegPath, QString* error)
{
    QProcess proc;
    proc.start(ffmpegPath, {"-version"});
    if(!proc.waitForStarted(5000))
    {
        if(error)
            *error= "FFmpeg was not found at '" + ffmpegPath
                    + "'. Install FFmpeg (with libx264, aac and the subtitles "
                      "filter) to render tutorial videos.";
        return false;
    }
    proc.waitForFinished(5000);
    if(proc.exitCode() != 0)
    {
        if(error)
            *error= "FFmpeg at '" + ffmpegPath + "' could not be executed.";
        return false;
    }
    return true;
}

QString VideoRenderer::buildAudioFilter(const QList<CommentAudio>& audios,
                                        bool keepOriginalAudio, double gain,
                                        QString* mixedLabel)
{
    QStringList parts;
    QStringList inputs;
    QString baseLabel;

    // Base track: the source audio, or synthesized silence when the original
    // audio is disabled (so the mix always has a valid first input).
    if(keepOriginalAudio)
        baseLabel= "[0:a]";
    else
    {
        baseLabel= "[silence]";
        parts << "anullsrc=r=48000:cl=stereo[silence]";
    }
    inputs.append(baseLabel);

    int i= 0;
    for(const auto& a : audios)
    {
        // Narration inputs are ffmpeg inputs 1..N (input 0 is the source video).
        const QString in= QString("[%1:a]").arg(1 + i);
        const QString out= QString("[n%1]").arg(i);
        const QString delay= QString::number(a.startMs) + "|0";
        parts << in + "adelay=" + delay
              + (gain > 0 ? QString(",volume=%1").arg(gain) : QString())
              + out;
        inputs.append(out);
        ++i;
    }

    const int channels= audios.size() + 1;
    const QString mixOut= "[mixed]";
    parts << inputs.join("")
             + QString("amix=inputs=%1:duration=first:normalize=0%2")
                 .arg(channels)
                 .arg(mixOut);

    if(mixedLabel)
        *mixedLabel= mixOut;
    return parts.join(";");
}

QStringList VideoRenderer::buildCommand(const RenderOptions& opts,
                                        const QList<CommentAudio>& audios)
{
    // Probe the source for an audio stream; fall back to silence if it has none
    // (otherwise "[0:a]" would be an undefined label and ffmpeg would fail).
    bool sourceHasAudio= true;
    {
        QProcess probe;
        probe.start("ffprobe",
                    {"-v", "error", "-select_streams", "a",
                     "-show_entries", "stream=index",
                     "-of", "csv=p=0", opts.sourceVideo});
        if(probe.waitForStarted(5000) && probe.waitForFinished(10000))
            sourceHasAudio= !probe.readAllStandardOutput().trimmed().isEmpty();
    }
    const bool keepAudio= opts.keepOriginalAudio && sourceHasAudio;

    // Build the narration mix.
    QString mixedLabel;
    QString audioFilter= buildAudioFilter(audios, keepAudio, opts.narrationGain,
                                          &mixedLabel);

    // Build the video chain (optionally with burned-in subtitles).
    QString videoChain;
    QString videoOut= "[v]";
    if(opts.embedSubtitles && !opts.srtFile.isEmpty())
    {
        const QString escaped= TtsEngine::escapeFilterPath(opts.srtFile);
        videoChain= QString("[0:v]subtitles='%1'[v]").arg(escaped);
    }
    else
    {
        videoChain= "[0:v]copy[v]";
    }

    QString filterComplex= videoChain;
    if(!audios.isEmpty())
        filterComplex += ";" + audioFilter;

    QStringList cmd;
    cmd << opts.ffmpegPath
        << "-y"
        << "-hide_banner"
        << "-loglevel"
        << "error";

    cmd << "-i" << opts.sourceVideo;
    for(const auto& a : audios)
        cmd << "-i" << a.audioFile;

    cmd << "-filter_complex" << filterComplex;
    cmd << "-map" << "[v]";
    cmd << "-map" << mixedLabel;
    cmd << "-c:v" << "libx264"
        << "-pix_fmt" << "yuv420p"
        << "-preset" << "medium"
        << "-crf" << "23";
    cmd << "-c:a" << "aac"
        << "-b:a" << "192k";
    cmd << "-movflags" << "+faststart";
    cmd << "-shortest";
    cmd << opts.outputVideo;
    return cmd;
}

bool VideoRenderer::isRunning() const
{
    return m_process != nullptr && m_process->state() != QProcess::NotRunning;
}

void VideoRenderer::cancel()
{
    m_cancelled= true;
    if(m_process && m_process->state() != QProcess::NotRunning)
        m_process->kill();
}

bool VideoRenderer::renderSync(const RenderOptions& opts,
                               const QList<CommentAudio>& audios, QString* error)
{
    if(audios.isEmpty())
    {
        if(error)
            *error= "No narration audio to render. Generate comment audio first.";
        return false;
    }

    // Validate inputs up front so we fail fast with actionable messages.
    if(!QFileInfo::exists(opts.sourceVideo))
    {
        if(error)
            *error= "Source video not found: " + opts.sourceVideo;
        return false;
    }
    for(const auto& a : audios)
        if(!QFileInfo::exists(a.audioFile))
        {
            if(error)
                *error= "Narration audio not found: " + a.audioFile
                        + ". Generate the comment audio first.";
            return false;
        }
    if(opts.embedSubtitles && !opts.srtFile.isEmpty()
        && !QFileInfo::exists(opts.srtFile))
    {
        if(error)
            *error= "Subtitle file not found: " + opts.srtFile;
        return false;
    }

    QString ffError;
    if(!isAvailable(opts.ffmpegPath, &ffError))
    {
        if(error)
            *error= ffError;
        return false;
    }

    // Render to a temporary file in the same directory, then atomically move
    // it to the final location only on success.
    QDir().mkpath(QDir(opts.outputVideo).absolutePath());
    QTemporaryFile temp(QDir(opts.outputVideo).absoluteFilePath(".render_XXXXXX.mp4"));
    temp.setAutoRemove(false);
    if(!temp.open())
    {
        if(error)
            *error= "Could not create temporary render file.";
        return false;
    }
    const QString tempPath= temp.fileName();
    temp.close();

    RenderOptions tempOpts= opts;
    tempOpts.outputVideo= tempPath;
    const QStringList cmd= buildCommand(tempOpts, audios);

    QProcess proc;
    proc.start(cmd.first(), cmd.mid(1));
    if(!proc.waitForStarted(10000))
    {
        QFile::remove(tempPath);
        if(error)
            *error= "Failed to start FFmpeg: " + proc.errorString();
        return false;
    }
    if(!proc.waitForFinished(60 * 60 * 1000))
    {
        proc.kill();
        QFile::remove(tempPath);
        if(error)
            *error= "FFmpeg timed out.";
        return false;
    }

    if(proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0)
    {
        QFile::remove(tempPath);
        if(error)
            *error= QString("FFmpeg failed (code %1): %2")
                        .arg(proc.exitCode())
                        .arg(QString::fromUtf8(proc.readAllStandardError()).trimmed());
        return false;
    }

    if(QFile::exists(opts.outputVideo))
        QFile::remove(opts.outputVideo);
    if(!QFile::rename(tempPath, opts.outputVideo))
    {
        QFile::remove(tempPath);
        if(error)
            *error= "Could not move rendered file to: " + opts.outputVideo;
        return false;
    }
    return true;
}

void VideoRenderer::renderAsync(const RenderOptions& opts,
                                const QList<CommentAudio>& audios)
{
    if(isRunning())
    {
        emit renderFinished(false, "A render is already in progress.");
        return;
    }

    if(audios.isEmpty())
    {
        emit renderFinished(false,
                            "No narration audio to render. Generate comment audio first.");
        return;
    }
    if(!QFileInfo::exists(opts.sourceVideo))
    {
        emit renderFinished(false, "Source video not found: " + opts.sourceVideo);
        return;
    }
    for(const auto& a : audios)
        if(!QFileInfo::exists(a.audioFile))
        {
            emit renderFinished(false, "Narration audio not found: " + a.audioFile);
            return;
        }

    QString ffError;
    if(!isAvailable(opts.ffmpegPath, &ffError))
    {
        emit renderFinished(false, ffError);
        return;
    }

    QDir().mkpath(QDir(opts.outputVideo).absolutePath());
    QTemporaryFile temp(QDir(opts.outputVideo).absoluteFilePath(".render_XXXXXX.mp4"));
    temp.setAutoRemove(false);
    if(!temp.open())
    {
        emit renderFinished(false, "Could not create temporary render file.");
        return;
    }
    m_tempOutput= temp.fileName();
    temp.close();
    m_finalOutput= opts.outputVideo;
    m_cancelled= false;

    RenderOptions tempOpts= opts;
    tempOpts.outputVideo= m_tempOutput;
    const QStringList cmd= buildCommand(tempOpts, audios);

    m_process= new QProcess(this);
    connect(m_process, &QProcess::finished, this, &VideoRenderer::onProcessFinished);
    emit renderStarted();
    m_process->start(cmd.first(), cmd.mid(1));
}

void VideoRenderer::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    auto* proc= m_process;
    m_process= nullptr;

    if(m_cancelled)
    {
        QFile::remove(m_tempOutput);
        emit renderFinished(false, "Render cancelled.");
        proc->deleteLater();
        return;
    }

    if(status != QProcess::NormalExit || exitCode != 0)
    {
        QFile::remove(m_tempOutput);
        emit renderFinished(false,
                            QString("FFmpeg failed (code %1): %2")
                                .arg(exitCode)
                                .arg(QString::fromUtf8(proc->readAllStandardError()).trimmed()));
        proc->deleteLater();
        return;
    }

    if(QFile::exists(m_finalOutput))
        QFile::remove(m_finalOutput);
    if(!QFile::rename(m_tempOutput, m_finalOutput))
    {
        QFile::remove(m_tempOutput);
        emit renderFinished(false,
                            "Could not move rendered file to: " + m_finalOutput);
        proc->deleteLater();
        return;
    }

    emit renderFinished(true, "Render complete: " + m_finalOutput);
    proc->deleteLater();
}
