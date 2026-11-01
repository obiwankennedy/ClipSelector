#ifndef TIMELINEEFFECTTOKEN_H
#define TIMELINEEFFECTTOKEN_H

#include <QColor>
#include <QQuickPaintedItem>
#include <QString>

#include "timedevent.h"
#include "timelinetokenbase.h"

// Token representing a timed effect (VideoTimedEvent) on the timeline.
//
// It mirrors TimeLineToken for clips, but is backed by a VideoTimedEvent
// instead of a ClipInfo. Its color follows the event type, and it exposes a
// single `value` input that maps to the effect's editable property:
//   - AudioCommentEvent -> text
//   - SpeedUpEvent      -> timeFactor
class TimeLineEffectToken : public TimeLineTokenBase
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(VideoTimedEvent* timedEvent READ timedEvent WRITE setTimedEvent NOTIFY timedEventChanged REQUIRED FINAL)
    Q_PROPERTY(QString value READ value WRITE setValue NOTIFY valueChanged FINAL)

public:
    explicit TimeLineEffectToken(QQuickItem* parent= nullptr);
    ~TimeLineEffectToken() override= default;

    VideoTimedEvent* timedEvent() const;
    void setTimedEvent(VideoTimedEvent* newTimedEvent);

    QString value() const;
    void setValue(const QString& newValue);

    QString labelText() const override;

signals:
    void timedEventChanged();
    void valueChanged();

private:
    QPointer<VideoTimedEvent> m_timedEvent;
};

#endif // TIMELINEEFFECTTOKEN_H
