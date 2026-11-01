#ifndef VIDEOEFFECT_H
#define VIDEOEFFECT_H

#include <QAbstractListModel>
#include <QJsonObject>
#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QUuid>

#include "timedevent.h"

class CutEvent : public VideoTimedEvent
{
    Q_OBJECT
public:
    CutEvent()
        : VideoTimedEvent(VideoTimedEvent::EventType::CUTEVENT){

          };
};

// A segment of the source video that is played at a different speed.
// timeFactor is the playback speed: 1 = no change, 2 = twice as fast,
// 0.5 = twice as long.
class SpeedUpEvent : public VideoTimedEvent
{
    Q_OBJECT
    Q_PROPERTY(qreal timeFactor READ timeFactor WRITE setTimeFactor NOTIFY timeFactorChanged FINAL)
public:
    SpeedUpEvent()
        : VideoTimedEvent(VideoTimedEvent::EventType::SPEEDUPEVENT){

          };
    qreal timeFactor() const;
    void setTimeFactor(qreal newTimeFactor);

    void saveData(QJsonObject& obj) const override { obj["timeFactor"]= m_timeFactor; };
    void readData(QJsonObject& obj) override { m_timeFactor= obj["timeFactor"].toDouble(m_timeFactor); };

signals:
    void timeFactorChanged();

private:
    qreal m_timeFactor{1.0};
};

// A spoken comment attached to a position of the video. When `text` is set,
// the TutorialController generates the matching Piper audio file (audioFile)
// and adapts the event duration to the length of the audio.
class AudioCommentEvent : public VideoTimedEvent
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged FINAL)
    Q_PROPERTY(QString id READ id CONSTANT FINAL)
    Q_PROPERTY(QString audioFile READ audioFile WRITE setAudioFile NOTIFY audioFileChanged FINAL)
public:
    AudioCommentEvent()
        : VideoTimedEvent(VideoTimedEvent::EventType::AUDIOCOMMENT)
        , m_id{QUuid::createUuid().toString(QUuid::WithoutBraces)}
          {
          };
    QString text() const;
    void setText(const QString& newText);

    // Stable identifier, used to name the audio folder of the generated file.
    QString id() const;
    QString audioFile() const;
    void setAudioFile(const QString& newAudioFile);

    void saveData(QJsonObject& obj) const override
    {
        obj["text"]= m_text;
        obj["id"]= m_id;
        obj["audioFile"]= m_audioFile;
    }
    void readData(QJsonObject& obj) override
    {
        m_text= obj["text"].toString(m_text);
        m_id= obj["id"].toString(m_id);
        m_audioFile= obj["audioFile"].toString();
    }
signals:
    void textChanged();
    void audioFileChanged();

private:
    QString m_text;
    QString m_id;
    QString m_audioFile;
};

// List model of VideoEffect, keeping insertion order.
class VideoEffectModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
public:
    enum DataRole
    {
        Id= Qt::UserRole + 1,
        TypeRole,
        // Human-readable type name for table views ("Cut", "Speed up", ...).
        TypeNameRole,
        BeginRole,
        EndRole,
        DurationRole,
        ColorRole,
        ObjectRole,
        // Generated audio file for AudioCommentEvent ("" for other types).
        AudioFileRole
    };
    Q_ENUM(DataRole)

    VideoEffectModel(QObject* parent= nullptr);
    ~VideoEffectModel() override;

    int rowCount(const QModelIndex& parent= QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    QHash<int, QByteArray> roleNames() const override;

    // Access an event by row (usable from QML).
    Q_INVOKABLE VideoTimedEvent* effectAt(int row) const;

    // Create a new event of the given type (owned by the caller).
    static VideoTimedEvent* createEvent(VideoTimedEvent::EventType type);

    void addEffect(VideoTimedEvent::EventType type, qint64 start, qint64 end);
    void removeEffect(int row);
    void clear();
    void reset(const QList<VideoTimedEvent*>& infos);

    const std::vector<std::unique_ptr<VideoTimedEvent>>& allEffects() const;

signals:
    void effectsChanged();

private:
    int rowOf(const VideoTimedEvent* event) const;
    // Forward the event's own property changes to dataChanged() so the QML
    // timeline tokens and any auto-save listeners observe them.
    void connectEvent(VideoTimedEvent* event);

    std::vector<std::unique_ptr<VideoTimedEvent>> m_data;
};

#endif // VIDEOEFFECT_H
