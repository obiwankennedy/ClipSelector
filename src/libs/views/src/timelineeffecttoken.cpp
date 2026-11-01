#include "timelineeffecttoken.h"

#include "timedevent.h"
#include "videoeffect.h"

#include <QColor>

TimeLineEffectToken::TimeLineEffectToken(QQuickItem* parent) : TimeLineTokenBase(parent) {}

VideoTimedEvent* TimeLineEffectToken::timedEvent() const
{
    return m_timedEvent;
}

void TimeLineEffectToken::setTimedEvent(VideoTimedEvent* newTimedEvent)
{
    if(m_timedEvent == newTimedEvent)
        return;
    if(m_timedEvent)
        disconnect(m_timedEvent, nullptr, this, nullptr);

    m_timedEvent= newTimedEvent;
    emit timedEventChanged();

    if(!m_timedEvent)
        return;

    // The color follows the type of the event.
    setColor(m_timedEvent->color());

    // Keep the `value` input in sync with the effect's editable property.
    connect(m_timedEvent, &QObject::destroyed, this, [this]() { update(); });
    VideoTimedEvent* event= m_timedEvent;
    if(auto speed= dynamic_cast<SpeedUpEvent*>(event))
        connect(speed, &SpeedUpEvent::timeFactorChanged, this,
                [this]()
                {
                    emit valueChanged();
                    update();
                });
    if(auto comment= dynamic_cast<AudioCommentEvent*>(event))
        connect(comment, &AudioCommentEvent::textChanged, this,
                [this]()
                {
                    emit valueChanged();
                    update();
                });
    update();
}

QString TimeLineEffectToken::value() const
{
    const VideoTimedEvent* event= m_timedEvent;
    if(auto speed= dynamic_cast<const SpeedUpEvent*>(event))
        return QString::number(speed->timeFactor(), 'f', 2);
    if(auto comment= dynamic_cast<const AudioCommentEvent*>(event))
        return comment->text();
    return {};
}

void TimeLineEffectToken::setValue(const QString& newValue)
{
    // Writing the property fires the event's changed signal, which is already
    // connected to emit valueChanged() and repaint.
    VideoTimedEvent* event= m_timedEvent;
    if(auto speed= dynamic_cast<SpeedUpEvent*>(event))
    {
        // Reject invalid speed factors (NaN, 0 or negative).
        bool ok= false;
        const double factor= newValue.trimmed().toDouble(&ok);
        if(!ok || qIsNaN(factor) || factor <= 0.0)
            return;
        speed->setTimeFactor(factor);
    }
    else if(auto comment= dynamic_cast<AudioCommentEvent*>(event))
        comment->setText(newValue);
}

QString TimeLineEffectToken::labelText() const
{
    const VideoTimedEvent* event= m_timedEvent;
    if(!event)
        return {};
    switch(event->type())
    {
    case VideoTimedEvent::EventType::CUTEVENT:
        return QStringLiteral("cut");
    case VideoTimedEvent::EventType::SPEEDUPEVENT:
        return QString::number(static_cast<const SpeedUpEvent*>(event)->timeFactor(), 'f', 2) + "x";
    case VideoTimedEvent::EventType::AUDIOCOMMENT:
    {
        const auto* comment= static_cast<const AudioCommentEvent*>(event);
        QString text= comment->text();
        if(text.length() > 20)
            text= text.left(19) + QStringLiteral("…");
        return text;
    }
    }
    return {};
}
