#include "effectplayback.h"

#include "videoeffect.h"

#include <QFileInfo>
#include <QMediaPlayer>
#include <QSoundEffect>
#include <QUrl>

namespace
{
// Ignore cut re-triggers right after a jump: while the backend decodes from
// the previous keyframe it can report positions inside the cut again.
constexpr qint64 CutJumpCooldownMs= 300;
} // namespace

EffectPlayback::EffectPlayback(VideoEffectModel* effects, QObject* parent)
    : QObject(parent)
    , m_effects(effects)
    , m_sound(new QSoundEffect(this))
{
    if(m_effects)
    {
        // Keep the caches in sync with any model / property change.
        connect(m_effects, &QAbstractListModel::rowsInserted, this, &EffectPlayback::refresh);
        connect(m_effects, &QAbstractListModel::rowsRemoved, this, &EffectPlayback::refresh);
        connect(m_effects, &QAbstractListModel::modelReset, this, &EffectPlayback::refresh);
        connect(m_effects, &QAbstractListModel::dataChanged, this, &EffectPlayback::refresh);
    }
    refresh();
}

QMediaPlayer* EffectPlayback::player() const
{
    return m_player;
}

QSoundEffect* EffectPlayback::commentSound() const
{
    return m_sound;
}

void EffectPlayback::setPlayer(QMediaPlayer* player)
{
    if(m_player == player)
        return;

    if(m_player)
    {
        disconnect(m_player, nullptr, this, nullptr);
        m_player->setPlaybackRate(1.0f);
    }

    m_player= player;
    m_appliedRate= 1.0;
    m_lastPosition= 0;

    if(!m_player)
        return;

    connect(m_player, &QMediaPlayer::positionChanged, this, &EffectPlayback::onPositionChanged);
    connect(m_player, &QMediaPlayer::playbackStateChanged, this, &EffectPlayback::onPlaybackStateChanged);
    connect(m_player, &QMediaPlayer::sourceChanged, this, [this] {
        // A new video: start from a clean state.
        m_sound->stop();
        m_lastPosition= 0;
        m_appliedRate= 1.0;
        m_lastJump.invalidate();
        m_player->setPlaybackRate(1.0f);
    });
}

void EffectPlayback::refresh()
{
    m_cuts.clear();
    m_speeds.clear();
    m_comments.clear();

    if(!m_effects)
        return;

    for(const auto& e : m_effects->allEffects())
    {
        switch(e->type())
        {
        case VideoTimedEvent::EventType::CUTEVENT:
            // Only a real segment (end > begin) removes time from the video.
            if(e->end() > e->begin())
                m_cuts.append({e->begin(), e->end()});
            break;
        case VideoTimedEvent::EventType::SPEEDUPEVENT:
        {
            auto* speed= static_cast<SpeedUpEvent*>(e.get());
            const qreal factor= speed->timeFactor();
            if(factor > 0.0 && !qFuzzyCompare(factor, 1.0))
                m_speeds.append({e->begin(), e->end(), factor});
            break;
        }
        case VideoTimedEvent::EventType::AUDIOCOMMENT:
        {
            auto* comment= static_cast<AudioCommentEvent*>(e.get());
            if(!comment->audioFile().isEmpty())
                m_comments.append({comment->begin(), comment->audioFile()});
            break;
        }
        }
    }
}

void EffectPlayback::onPositionChanged(qint64 position)
{
    const qint64 previous= m_lastPosition;
    m_lastPosition= position;

    if(!m_player || m_player->playbackState() != QMediaPlayer::PlayingState)
        return;

    // Cuts: when the playhead is inside a cut, jump to its end (with a
    // cooldown so an in-flight seek does not re-trigger itself).
    for(const auto& cut : m_cuts)
    {
        if(position >= cut.begin && position < cut.end)
        {
            if(m_lastJump.isValid() && m_lastJump.elapsed() < CutJumpCooldownMs)
                return;
            m_lastJump.start();
            m_lastPosition= cut.end;
            m_player->setPosition(cut.end);
            return;
        }
    }

    // Speed: play the current segment at its factor.
    qreal rate= 1.0;
    for(const auto& speed : m_speeds)
    {
        if(position >= speed.begin && position < speed.end)
        {
            rate= speed.factor;
            break;
        }
    }
    if(!qFuzzyCompare(rate, m_appliedRate))
    {
        m_appliedRate= rate;
        m_player->setPlaybackRate(static_cast<float>(rate));
    }

    // Comments: start the audio when the playhead crosses the begin.
    for(const auto& comment : m_comments)
    {
        if(previous < comment.begin && position >= comment.begin
           && QFileInfo::exists(comment.audioFile))
        {
            m_sound->setSource(QUrl::fromLocalFile(comment.audioFile));
            m_sound->play();
        }
    }
}

void EffectPlayback::onPlaybackStateChanged(QMediaPlayer::PlaybackState state)
{
    if(state != QMediaPlayer::PlayingState)
    {
        m_sound->stop();
        if(state == QMediaPlayer::StoppedState)
        {
            m_lastPosition= 0;
            m_lastJump.invalidate();
            if(!qFuzzyCompare(m_appliedRate, 1.0))
            {
                m_appliedRate= 1.0;
                m_player->setPlaybackRate(1.0f);
            }
        }
    }
}
