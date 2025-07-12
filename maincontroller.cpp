#include "maincontroller.h"

#include <QFileInfo>
#include <QSaveFile>

#include "utils.h"

MainController::MainController(QObject* parent)
    : QObject{parent}, m_clipModel{new ClipModel}, m_musics{new AudioFileModel}, m_proxyMusic{new FilteredModel}
{
    connect(this, &MainController::fileNameChanged, this, &MainController::reset);

    Utils::readAudioList("/home/renaud/documents/playlist_official.apl", m_musics.get());

    m_proxyMusic->setSourceModel(m_musics.get());
}

QString MainController::fileName() const
{
    return m_fileName;
}

void MainController::setFileName(const QString& newFileName)
{
    if(m_fileName == newFileName)
        return;
    m_fileName= newFileName;
    emit fileNameChanged();
}

ClipModel* MainController::clipModel() const
{
    return m_clipModel.get();
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
    m_clipModel->reset();
    if(m_fileName.endsWith(".json"))
        openJson(m_fileName);
}

void MainController::openJson(const QString& uri)
{
    Utils::readJson(QUrl::fromUserInput(uri).toLocalFile(), this);
}

void MainController::saveModel(const QString& uri) const
{
    auto const& data= Utils::model2Json(m_fileName, m_clipModel.get());

    auto filePath= QUrl(uri).toLocalFile();

    if(!filePath.endsWith(".json"))
        filePath+= ".json";

    QFileInfo inf(filePath);

    qDebug() << inf.isWritable();

    QSaveFile file(filePath);

    qDebug() << filePath;

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
    auto infos= m_clipModel->allData();

    for(auto const& info : infos)
    {
        auto ranges= info.ranges;
        bool ok;
        auto pos= ranges->closiestPositionInClip(m_position, &ok);

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
