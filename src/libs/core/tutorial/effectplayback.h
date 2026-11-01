#ifndef EFFECTPLAYBACK_H
#define EFFECTPLAYBACK_H

#include <QElapsedTimer>
#include <QMediaPlayer>
#include <QObject>
#include <QVector>

class QSoundEffect;
class VideoEffectModel;

// Makes the live playback follow the timeline effects:
//  - a CutEvent segment is skipped: when the playhead reaches the cut, the
//    video jumps to the end of the cut;
//  - a SpeedUpEvent segment is played at its timeFactor (1 = normal speed,
//    2 = twice as fast, 0.5 = twice as long);
//  - an AudioCommentEvent audio starts when the playhead reaches its begin.
class EffectPlayback : public QObject
{
    Q_OBJECT
public:
    explicit EffectPlayback(VideoEffectModel* effects, QObject* parent= nullptr);

    // Bind (or unbind, with nullptr) the media player whose playback must
    // follow the effects.
    Q_INVOKABLE void setPlayer(QMediaPlayer* player);
    Q_INVOKABLE QMediaPlayer* player() const;

    // The sound effect used for the comment audios (exposed for tests).
    Q_INVOKABLE QSoundEffect* commentSound() const;

private:
    struct Cut
    {
        qint64 begin= 0;
        qint64 end= 0;
    };
    struct Speed
    {
        qint64 begin= 0;
        qint64 end= 0;
        qreal factor= 1.0;
    };
    struct Comment
    {
        qint64 begin= 0;
        QString audioFile;
    };

    // Rebuild the effect caches from the model.
    void refresh();
    void onPositionChanged(qint64 position);
    void onPlaybackStateChanged(QMediaPlayer::PlaybackState state);

    VideoEffectModel* m_effects= nullptr;
    QMediaPlayer* m_player= nullptr;
    QSoundEffect* m_sound= nullptr;
    qreal m_appliedRate= 1.0;
    qint64 m_lastPosition= 0;
    // Cooldown after a cut jump: the backend may report positions before the
    // seek target while it decodes from the previous keyframe, which must not
    // re-trigger the jump.
    QElapsedTimer m_lastJump;
    QVector<Cut> m_cuts;
    QVector<Speed> m_speeds;
    QVector<Comment> m_comments;
};

#endif // EFFECTPLAYBACK_H
