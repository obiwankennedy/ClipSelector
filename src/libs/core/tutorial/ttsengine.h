#ifndef TTSENGINE_H
#define TTSENGINE_H

#include <QObject>
#include <QProcess>
#include <QStringList>

class TtsEngine : public QObject
{
    Q_OBJECT
public:
    // Describes an installed Piper voice.
    struct Voice
    {
        QString name; // file stem, e.g. "en_US-amy-medium"
        QString language; // e.g. "en" (first token of the locale)
        QString modelPath; // absolute path to the .onnx file
    };

    // Result of a single generation request.
    struct GenerationResult
    {
        bool ok{false};
        QString audioFile; // absolute path of the produced wav (on success)
        qint64 durationMs{0};
        QString error;
    };

    explicit TtsEngine(QObject* parent= nullptr);

    // --- Piper environment configuration ---
    QString scriptPath() const;
    void setScriptPath(const QString& script);
    QString pythonPath() const;
    void setPythonPath(const QString& python);
    QString modelDirectory() const;
    void setModelDirectory(const QString& dir);

    // Languages and voices as currently available from the model directory.
    QStringList languages() const;
    // Voices compatible with a language (empty string -> all voices).
    QStringList voicesForLanguage(const QString& lang) const;
    // All discovered voices.
    QList<Voice> voices() const;

    // True when the script, the python interpreter and at least one model are
    // present. `details` (if non-null) receives a human readable problem list.
    bool isAvailable(QString* details= nullptr) const;

    // The exact command line that WOULD be executed for a single text. Exposed
    // for tests so the integration contract with gen_audio_files.py can be
    // verified without running Piper.
    //
    // gen_audio_files.py CLI:  <python> <script> -t <json-array-of-text> -d <dest> -l <lang>
    // It writes <dest>/file_00.wav. The destination directory must exist.
    QStringList buildCommand(const QString& text, const QString& destDir,
                             const QString& lang) const;

    // Generate speech for a single comment text. Non-blocking: emits
    // generationFinished(). `destDir` must already exist (create it with
    // ensureDestDir()).
    void generateAsync(const QString& text, const QString& destDir,
                       const QString& lang);
    // Synchronous variant (used by tests / rendering preparation).
    GenerationResult generateSync(const QString& text, const QString& destDir,
                                  const QString& lang);

    // Ensure destDir exists. Returns true on success.
    bool ensureDestDir(const QString& destDir) const;

    // Probe the duration (ms) of an audio file using ffprobe. Returns 0 and
    // sets *ok=false when the file is missing or ffprobe is unavailable.
    static bool audioDurationMs(const QString& filePath, qint64* durationMs,
                                QString* error= nullptr);

    // Escape a path for use inside an ffmpeg filter graph / subtitles filter.
    static QString escapeFilterPath(const QString& path);

signals:
    void generationStarted(const QString& text);
    void generationFinished(const QString& text, const GenerationResult& result);
    void generationError(const QString& text, const QString& error);

private:
    void onFinished(int exitCode, QProcess::ExitStatus status);

    QString m_scriptPath;
    QString m_pythonPath;
    QString m_modelDirectory;

    QProcess* m_process{nullptr};
    QString m_pendingText;
    QString m_pendingDestDir;
    QString m_pendingLang;
};

#endif // TTSENGINE_H
