#include "timedevent.h"

VideoTimedEvent::VideoTimedEvent(VideoTimedEvent::EventType type, QObject* parent) : QObject(parent), m_type{type} {}

qint64 VideoTimedEvent::begin() const
{
    return m_begin;
}

void VideoTimedEvent::setBegin(qint64 newBegin)
{
    if(m_begin == newBegin)
        return;
    m_begin= newBegin;
    emit beginChanged();
}

qint64 VideoTimedEvent::end() const
{
    return m_end;
}

void VideoTimedEvent::setEnd(qint64 newEnd)
{
    if(m_end == newEnd)
        return;
    m_end= newEnd;
    emit endChanged();
}

qint64 VideoTimedEvent::duration() const
{
    return m_end - m_begin;
}

QColor VideoTimedEvent::color() const
{
    switch(m_type)
    {
    case EventType::CUTEVENT:
        return Qt::red;
    case EventType::SPEEDUPEVENT:
        return Qt::cyan;
    case EventType::AUDIOCOMMENT:
        return Qt::magenta;
    }
    return Qt::black;
}

VideoTimedEvent::EventType VideoTimedEvent::type() const
{
    return m_type;
}
