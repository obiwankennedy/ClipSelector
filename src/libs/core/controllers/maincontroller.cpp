#include "maincontroller.h"

#include <QAbstractItemModelTester>
#include <QFileInfo>
#include <QSaveFile>
#include <QSettings>

#include "utils.h"

MainController::MainController(QObject* parent)
    : QObject{parent}
    , m_clipModel{new ClipModel}
    , m_clipListModel{new ClipListModel}
    , m_musics{new AudioFileModel}
    , m_proxyMusic{new FilteredModel}
{
    new QAbstractItemModelTester(m_clipModel.get());
    m_clipListModel->setSourceModel(m_clipModel.get());
    connect(this, &MainController::videoFilePathChanged, this, &MainController::reset);

    Utils::readAudioList("/home/renaud/documents/playlist_official.apl", m_musics.get());

    m_proxyMusic->setSourceModel(m_musics.get());
    loadSettings();

    m_runner.reset(new QProcess());
    QProcessEnvironment env= QProcessEnvironment::systemEnvironment();
    env.insert("PATH", QString("%1:%2").arg("/home/renaud/www/scripts/venv/bin", env.value("PATH")));
    env.insert("VIRTUAL_ENV_PROMPT", QString("(venv)"));
    env.insert("VIRTUAL_ENV", QString("/home/renaud/www/scripts/venv"));
    m_runner->setProcessEnvironment(env);

    connect(m_runner.get(), &QProcess::started, this, [this]() { setBusy(true); });
    connect(m_runner.get(), &QProcess::finished, this, [this]() { setBusy(false); });

    connect(m_runner.get(), &QProcess::errorOccurred, this, [this]() { qDebug() << "ERROR while running ClipMake"; });

    connect(m_runner.get(), &QProcess::readyReadStandardError, this,
            [this]() { qDebug() << m_runner->readAllStandardError(); });
    connect(m_runner.get(), &QProcess::readyReadStandardOutput, this,
            [this]() { qDebug() << m_runner->readAllStandardOutput(); });
}

QString MainController::videoFilePath() const
{
    return m_videoFilePath;
}

void MainController::setVideoFilePath(const QString& newFileName)
{
    if(m_videoFilePath == newFileName)
        return;
    m_videoFilePath= newFileName;
    emit videoFilePathChanged();
}

ClipModel* MainController::clipModel() const
{
    return m_clipModel.get();
}

ClipListModel* MainController::clipListModel() const
{
    return m_clipListModel.get();
}

AudioFileModel* MainController::musics() const
{
    return m_musics.get();
}

FilteredModel* MainController::wantedMusics() const
{
    return m_proxyMusic.get();
}

void MainController::reset()
{
    m_clipModel->reset({});
    if(m_videoFilePath.endsWith(".json"))
        openJson(m_videoFilePath);
}

void MainController::openJson(const QString& uri)
{
    setProjectFilePath(QUrl::fromUserInput(uri).toLocalFile());
    Utils::readJson(QUrl::fromUserInput(uri).toLocalFile(), this);
}

void MainController::saveModel(const QString& uri) const
{
    auto const& data= Utils::model2Json(m_videoFilePath, m_clipModel.get());

    auto filePath= QUrl(uri).toLocalFile();

    if(!filePath.endsWith(".json"))
        filePath+= ".json";

    QSaveFile file(filePath);

    if(file.open(QIODevice::WriteOnly))
    {
        qDebug() << data << uri;
        file.write(data);
        file.commit();
    }
    else
    {
        qDebug() << file.errorString();
    }
}

void MainController::playScenes()
{
    emit play();
    m_position= 0;
    computePosition();
}

void MainController::setPattern(const QString& pattern)
{
    m_proxyMusic->setSearch(pattern);
}

void MainController::computePosition()
{
    auto const& infos= m_clipModel->allData();

    for(auto const& info : infos)
    {
        auto ranges= info->ranges();
        bool ok;
        auto pos= Utils::closiestPositionInClip(ranges->allRanges(), m_position, &ok);

        if(!ok)
            continue;

        if(m_position != pos)
        {
            m_position= pos;
            emit movePosition(pos);
            return;
        }

        if(ok)
            return;
    }
}

void MainController::loadSettings()
{
    QSettings settings("rolisteam", "ClipSelector");

    m_clipMakeScript
        = settings.value("clipmakerscript", "file:///home/renaud/www/scripts/08_videos/clipmaker.py").toUrl();
}
void MainController::writeSettings()
{
    QSettings settings("rolisteam", "ClipSelector");

    settings.setValue("clipmakerscript", m_clipMakeScript);
}

void MainController::runClipMaker(bool keepSize, bool label, bool merge, bool noMusic)
{
    if(m_runner->state() != QProcess::NotRunning)
        return;
    QStringList args;

    if(keepSize)
        args << "-k";
    if(label)
        args << "-l";
    if(merge)
        args << "-m";
    if(noMusic)
        args << "-n";

    args << "-c" << QUrl::fromUserInput(m_projectFilePath).toLocalFile();
    m_runner->start(m_clipMakeScript.toLocalFile(), args);
    // proc.setEnvironment();
}

int MainController::closiestPosition(int idx, qreal pos)
{
    auto const& infos= m_clipModel->allData();
    auto const& ranges= infos[idx]->ranges();
    bool ok;
    return Utils::closiestPositionInClip(ranges->allRanges(), pos, &ok);
}

void MainController::setMusic(int idx, const QString& song)
{
    m_clipModel->setData(m_clipModel->index(idx, 0), song, ClipModel::Music);
}

void MainController::setMusicOffset(int idx, int offset)
{
    m_clipModel->setData(m_clipModel->index(idx, 0), offset, ClipModel::Offset);
}

quint64 MainController::position() const
{
    return m_position;
}

void MainController::setPosition(quint64 newPosition)
{
    if(m_position == newPosition)
        return;
    m_position= newPosition;
    computePosition();
    emit positionChanged();
}

QUrl MainController::clipMakeScript() const
{
    return m_clipMakeScript;
}

void MainController::setClipMakeScript(const QUrl& newClipMakeScript)
{
    if(m_clipMakeScript == newClipMakeScript)
        return;
    m_clipMakeScript= newClipMakeScript;
    emit clipMakeScriptChanged();
}

bool MainController::busy() const
{
    return m_busy;
}

void MainController::setBusy(bool newBusy)
{
    if(m_busy == newBusy)
        return;
    m_busy= newBusy;
    emit busyChanged();
}

QStringList MainController::modeNames()
{
    return {QString::fromUtf8("MUSIC CLIP"), QString::fromUtf8("TUTORIAL")};
}

MainController::AppMode MainController::appMode() const
{
    return m_appMode;
}

void MainController::setAppMode(MainController::AppMode newAppMode)
{
    if(m_appMode == newAppMode)
        return;
    m_appMode= newAppMode;
    emit appModeChanged();
}

QString MainController::projectFilePath() const
{
    return m_projectFilePath;
}

void MainController::setProjectFilePath(const QString& newProjectFilePath)
{
    if(m_projectFilePath == newProjectFilePath)
        return;
    m_projectFilePath= newProjectFilePath;
    emit projectFilePathChanged();
}
