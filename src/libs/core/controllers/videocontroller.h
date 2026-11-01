#ifndef VIDEOCONTROLLER_H
#define VIDEOCONTROLLER_H

#include <QObject>
#include <QQmlEngine>

#include <memory>

#include "clipmodel.h"
#include "filteredmodel.h"

class VideoController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    // clang-format off
    Q_PROPERTY(ClipModel* clipModel READ clipModel CONSTANT)
    Q_PROPERTY(ClipListModel* clipListModel READ clipListModel CONSTANT)
    Q_PROPERTY(quint64 position READ position WRITE setPosition NOTIFY positionChanged FINAL)
    Q_PROPERTY(VideoController::PlayingMode playingMode READ playingMode WRITE setPlayingMode NOTIFY playingModeChanged FINAL)
    Q_PROPERTY(VideoController::PlayerStatus playerStatus READ playerStatus WRITE setPlayerStatus NOTIFY playerStatusChanged FINAL)
    Q_PROPERTY(bool playing READ playing NOTIFY playerStatusChanged FINAL)
    // clang-format on
public:
    enum class PlayingMode
    {
        NormalMode,
        ClipMode
    };
    Q_ENUM(PlayingMode)
    enum class PlayerStatus
    {
        Stopped,
        Playing,
        Paused
    };
    Q_ENUM(PlayerStatus)
    VideoController(QObject* parent= nullptr);

    ClipModel* clipModel() const;
    ClipListModel* clipListModel() const;

    quint64 position() const;
    void setPosition(quint64 newPosition);

    VideoController::PlayingMode playingMode() const;
    void setPlayingMode(VideoController::PlayingMode newPlayingMode);

    VideoController::PlayerStatus playerStatus() const;
    void setPlayerStatus(VideoController::PlayerStatus newPlayerStatus);

    bool playing() const;

public slots:
    void reset();
    void setMusic(int idx, const QString& song);
    void setMusicOffset(int idx, int offset);
    int closiestPosition(int idx, qreal pos);
    void play(VideoController::PlayingMode mode);
    void stop();
    void pause();
    void clearSelection();

signals:
    void videoFilePathChanged();
    void clipMakeScriptChanged();
    void movePosition(quint64 p);
    void positionChanged();
    void playingModeChanged();
    void playerStatusChanged();

private slots:
    void computePosition();
    void playScenes();

private:
    std::unique_ptr<ClipModel> m_clipModel;
    std::unique_ptr<ClipListModel> m_clipListModel;
    quint64 m_position{0};
    VideoController::PlayingMode m_playingMode{VideoController::PlayingMode::NormalMode};
    VideoController::PlayerStatus m_playerStatus{VideoController::PlayerStatus::Stopped};
};

#endif // VIDEOCONTROLLER_H
