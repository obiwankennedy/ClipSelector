#include "ttsengine.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStandardPaths>

#include <algorithm>

namespace
{
// Default location of the authoritative Piper integration script.
const char* kDefaultScript
    = "/home/renaud/documents/11_rolisteam/05_documentation/01_videos/v1.10/gen_audio_files.py";
// The script lives inside a directory that contains a .venv with Piper.
const char* kDefaultPython
    = "/home/renaud/documents/11_rolisteam/05_documentation/01_videos/v1.10/.venv/bin/python3";
// Piper models are kept next to the script.
const char* kDefaultModelDir
    = "/home/renaud/documents/11_rolisteam/05_documentation/01_videos/v1.10";
}

TtsEngine::TtsEngine(QObject* parent)
    : QObject(parent)
    , m_scriptPath(kDefaultScript)
    , m_pythonPath(kDefaultPython)
    , m_modelDirectory(kDefaultModelDir)
{
}

QString TtsEngine::scriptPath() const
{
    return m_scriptPath;
}

void TtsEngine::setScriptPath(const QString& script)
{
    m_scriptPath= script;
}

QString TtsEngine::pythonPath() const
{
    return m_pythonPath;
}

void TtsEngine::setPythonPath(const QString& python)
{
    m_pythonPath= python;
}

QString TtsEngine::modelDirectory() const
{
    return m_modelDirectory;
}

void TtsEngine::setModelDirectory(const QString& dir)
{
    m_modelDirectory= dir;
}

QList<TtsEngine::Voice> TtsEngine::voices() const
{
    QList<Voice> result;
    QDir dir(m_modelDirectory);
    if(!dir.exists())
        return result;

    const auto entries= dir.entryList({"./*.onnx"}, QDir::Files);
    for(const auto& entry : entries)
    {
        QFileInfo fi(dir, entry);
        if(!fi.suffix().compare("onnx", Qt::CaseInsensitive))
            continue;
        Voice v;
        v.name= fi.completeBaseName(); // e.g. en_US-amy-medium
        // Language = first token before the first underscore (en_US -> en).
        auto underscore= v.name.indexOf('_');
        v.language= underscore > 0 ? v.name.left(underscore) : v.name;
        v.modelPath= fi.absoluteFilePath();
        result.append(v);
    }
    std::sort(std::begin(result), std::end(result),
              [](const Voice& a, const Voice& b) { return a.name < b.name; });
    return result;
}

QStringList TtsEngine::languages() const
{
    QSet<QString> langs;
    for(const auto& v : voices())
        langs.insert(v.language);
    return QStringList(langs.values());
}

QStringList TtsEngine::voicesForLanguage(const QString& lang) const
{
    QStringList names;
    for(const auto& v : voices())
        if(lang.isEmpty() || v.language == lang)
            names.append(v.name);
    return names;
}

bool TtsEngine::isAvailable(QString* details) const
{
    QStringList problems;
    if(!QFileInfo::exists(m_scriptPath))
        problems << "Piper script not found: " + m_scriptPath;
    if(!QFileInfo::exists(m_pythonPath))
        problems << "Python interpreter not found: " + m_pythonPath;
    if(voices().isEmpty())
        problems << "No Piper voice models found in: " + m_modelDirectory;

    if(details)
        *details= problems.isEmpty() ? QString() : problems.join("\n");
    return problems.isEmpty();
}

QStringList TtsEngine::buildCommand(const QString& text, const QString& destDir,
                                    const QString& lang) const
{
    // gen_audio_files.py reads a JSON *array* of texts and writes file_00.wav
    // (zero padded) into the destination folder. The folder must already exist
    // (the script does not create it), which ensureDestDir() guarantees.
    QJsonArray texts;
    texts.append(text);
    const QString textJson= QString::fromUtf8(
        QJsonDocument(texts).toJson(QJsonDocument::Compact));

    QStringList args;
    args << m_scriptPath
         << "-t" << textJson
         << "-d" << destDir
         << "-l" << lang;
    return args;
}

void TtsEngine::generateAsync(const QString& text, const QString& destDir,
                              const QString& lang)
{
    if(m_process)
    {
        emit generationError(text, "A generation is already in progress.");
        return;
    }

    if(!ensureDestDir(destDir))
    {
        GenerationResult r;
        r.error= "Could not create destination directory: " + destDir;
        emit generationError(text, r.error);
        emit generationFinished(text, r);
        return;
    }

    m_pendingText= text;
    m_pendingDestDir= destDir;
    m_pendingLang= lang;

    m_process= new QProcess(this);
    connect(m_process, &QProcess::finished, this, &TtsEngine::onFinished);
    emit generationStarted(text);
    m_process->start(m_pythonPath, buildCommand(text, destDir, lang));
}

void TtsEngine::onFinished(int exitCode, QProcess::ExitStatus status)
{
    auto* proc= m_process;
    m_process= nullptr;

    const QString text= m_pendingText;
    const QString destDir= m_pendingDestDir;
    const QString audioFile= QDir(destDir).filePath("file_00.wav");

    GenerationResult result;

    if(status != QProcess::NormalExit || exitCode != 0)
    {
        result.ok= false;
        result.error= QString("Piper exited with code %1: %2")
                          .arg(exitCode)
                          .arg(QString::fromUtf8(proc->readAllStandardError()).trimmed());
        emit generationError(text, result.error);
        emit generationFinished(text, result);
        proc->deleteLater();
        return;
    }

    if(!QFileInfo::exists(audioFile))
    {
        result.ok= false;
        result.error= "Piper reported success but no audio file was produced at " + audioFile;
        emit generationError(text, result.error);
        emit generationFinished(text, result);
        proc->deleteLater();
        return;
    }

    QString durationError;
    qint64 duration= 0;
    if(!audioDurationMs(audioFile, &duration, &durationError))
    {
        result.ok= false;
        result.audioFile= audioFile;
        result.error= durationError;
        emit generationError(text, result.error);
        emit generationFinished(text, result);
        proc->deleteLater();
        return;
    }

    result.ok= true;
    result.audioFile= audioFile;
    result.durationMs= duration;
    emit generationFinished(text, result);
    proc->deleteLater();
}

TtsEngine::GenerationResult TtsEngine::generateSync(const QString& text,
                                                    const QString& destDir,
                                                    const QString& lang)
{
    GenerationResult result;

    if(!ensureDestDir(destDir))
    {
        result.error= "Could not create destination directory: " + destDir;
        return result;
    }

    QProcess proc;
    proc.start(m_pythonPath, buildCommand(text, destDir, lang));
    if(!proc.waitForStarted(15000))
    {
        result.error= "Failed to start Piper: " + proc.errorString();
        return result;
    }
    proc.waitForFinished(120000);

    const QString audioFile= QDir(destDir).filePath("file_00.wav");

    if(proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0)
    {
        result.ok= false;
        result.error= QString("Piper exited with code %1: %2")
                          .arg(proc.exitCode())
                          .arg(QString::fromUtf8(proc.readAllStandardError()).trimmed());
        return result;
    }

    if(!QFileInfo::exists(audioFile))
    {
        result.ok= false;
        result.error= "Piper reported success but no audio file was produced at " + audioFile;
        return result;
    }

    result.ok= true;
    result.audioFile= audioFile;
    if(!audioDurationMs(audioFile, &result.durationMs, &result.error))
        result.ok= false; // wav exists; duration unknown.
    return result;
}

bool TtsEngine::ensureDestDir(const QString& destDir) const
{
    return QDir().mkpath(destDir);
}

bool TtsEngine::audioDurationMs(const QString& filePath, qint64* durationMs,
                                QString* error)
{
    if(durationMs)
        *durationMs= 0;
    if(!QFileInfo::exists(filePath))
    {
        if(error)
            *error= "Audio file does not exist: " + filePath;
        return false;
    }

    QProcess proc;
    proc.start("ffprobe",
               {"-v", "error",
                "-show_entries", "format=duration",
                "-of", "default=noprint_wrappers=1:nokey=1",
                filePath});
    if(!proc.waitForStarted(10000))
    {
        if(error)
            *error= "ffprobe is not available (needed to measure audio length).";
        return false;
    }
    if(!proc.waitForFinished(15000))
    {
        if(error)
            *error= "ffprobe timed out while reading " + filePath;
        return false;
    }
    if(proc.exitCode() != 0)
    {
        if(error)
            *error= QString("ffprobe failed on %1: %2")
                        .arg(filePath)
                        .arg(QString::fromUtf8(proc.readAllStandardError()).trimmed());
        return false;
    }

    bool ok= false;
    double seconds= QString::fromUtf8(proc.readAllStandardOutput()).trimmed().toDouble(&ok);
    if(!ok)
    {
        if(error)
            *error= "Could not parse audio duration for " + filePath;
        return false;
    }
    if(durationMs)
        *durationMs= static_cast<qint64>(seconds * 1000.0 + 0.5);
    return true;
}

QString TtsEngine::escapeFilterPath(const QString& path)
{
    // For use inside a single-quoted ffmpeg filter argument: escape backslash,
    // colon and single quotes.
    QString escaped= path;
    escaped.replace("\\", "\\\\");
    escaped.replace(":", "\\:");
    escaped.replace("'", "\\'");
    return escaped;
}
