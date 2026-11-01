#ifndef TIMEDEVENT_H
#define TIMEDEVENT_H

#include <QColor>
#include <QObject>
#include <QtQmlIntegration>

class VideoTimedEvent : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(qint64 begin READ begin WRITE setBegin NOTIFY beginChanged FINAL)
    Q_PROPERTY(qint64 end READ end WRITE setEnd NOTIFY endChanged FINAL)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged FINAL)
    Q_PROPERTY(QColor color READ color CONSTANT)
    Q_PROPERTY(VideoTimedEvent::EventType type READ type CONSTANT)
public:
    enum class EventType
    {
        CUTEVENT,
        SPEEDUPEVENT,
        AUDIOCOMMENT
    };
    Q_ENUM(EventType)
    VideoTimedEvent(VideoTimedEvent::EventType type, QObject* parent= nullptr);
    qint64 begin() const;
    void setBegin(qint64 newBegin);
    qint64 end() const;
    void setEnd(qint64 newEnd);
    qint64 duration() const;
    QColor color() const;

    VideoTimedEvent::EventType type() const;

    virtual void saveData(QJsonObject& obj) const {};
    virtual void readData(QJsonObject& obj) {};

signals:
    void beginChanged();
    void endChanged();
    void durationChanged();
    void colorChanged();

private:
    qint64 m_begin{0};
    qint64 m_end{0};
    VideoTimedEvent::EventType m_type{VideoTimedEvent::EventType::AUDIOCOMMENT};
};

#endif // TIMEDEVENT_H
