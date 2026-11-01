#include "videocontroller.h"

#include "utils.h"
#include <QAbstractItemModelTester>

VideoController::VideoController(QObject* parent)
    : QObject{parent}, m_clipModel{new ClipModel}, m_clipListModel{new ClipListModel}
{
    new QAbstractItemModelTester(m_clipModel.get());
    m_clipListModel->setSourceModel(m_clipModel.get());
}

ClipModel* VideoController::clipModel() const
{
    return m_clipModel.get();
}

ClipListModel* VideoController::clipListModel() const
{
    return m_clipListModel.get();
}

void VideoController::reset()
{
    m_clipModel->reset({});
}

quint64 VideoController::position() const
{
    return m_position;
}

int VideoController::closiestPosition(int idx, qreal pos)
{
    auto const& infos= m_clipModel->allData();
    auto const& ranges= infos[idx]->ranges();
    bool ok;
    return Utils::closiestPositionInClip(ranges->allRanges(), pos, &ok);
}

void VideoController::play(PlayingMode mode)
{
    setPlayingMode(mode);
    setPlayerStatus(PlayerStatus::Playing);
    if(mode == PlayingMode::ClipMode)
        playScenes();
}

void VideoController::stop()
{
    setPlayerStatus(PlayerStatus::Stopped);
}

void VideoController::pause()
{
    setPlayerStatus(PlayerStatus::Paused);
}

void VideoController::clearSelection()
{
    m_clipModel->clearSelection();
}

void VideoController::setPosition(quint64 newPosition)
{
    if(m_position == newPosition)
        return;
    m_position= newPosition;
    computePosition();
    emit positionChanged();
}

void VideoController::playScenes()
{
    m_position= 0;
    computePosition();
}

void VideoController::computePosition()
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

void VideoController::setMusic(int idx, const QString& song)
{
    m_clipModel->setData(m_clipModel->index(idx, 0), song, ClipModel::Music);
}

void VideoController::setMusicOffset(int idx, int offset)
{
    m_clipModel->setData(m_clipModel->index(idx, 0), offset, ClipModel::Offset);
}

VideoController::PlayingMode VideoController::playingMode() const
{
    return m_playingMode;
}

void VideoController::setPlayingMode(VideoController::PlayingMode newPlayingMode)
{
    if(m_playingMode == newPlayingMode)
        return;
    m_playingMode= newPlayingMode;
    emit playingModeChanged();
}

VideoController::PlayerStatus VideoController::playerStatus() const
{
    return m_playerStatus;
}

void VideoController::setPlayerStatus(VideoController::PlayerStatus newPlayerStatus)
{
    if(m_playerStatus == newPlayerStatus)
        return;
    m_playerStatus= newPlayerStatus;
    emit playerStatusChanged();
}

bool VideoController::playing() const
{
    return m_playerStatus == PlayerStatus::Playing;
}
