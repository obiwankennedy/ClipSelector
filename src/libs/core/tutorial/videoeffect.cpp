#include "videoeffect.h"

#include <QUuid>

#include <algorithm>

VideoEffectModel::VideoEffectModel(QObject* parent) : QAbstractListModel(parent) {}

VideoEffectModel::~VideoEffectModel() {}

int VideoEffectModel::rowCount(const QModelIndex& parent) const
{
    if(parent.isValid())
        return 0;
    return m_data.size();
}

QVariant VideoEffectModel::data(const QModelIndex& index, int role) const
{
    if(!index.isValid() || index.row() < 0 || index.row() >= m_data.size())
        return {};

    auto const& e= m_data.at(index.row());
    switch(role)
    {
    case Qt::DisplayRole:
    case DurationRole:
        return e->duration();
    case Id:
        return index.row();
    case TypeRole:
        return QVariant::fromValue(e->type());
    case TypeNameRole:
        switch(e->type())
        {
        case VideoTimedEvent::EventType::CUTEVENT:
            return QStringLiteral("Cut");
        case VideoTimedEvent::EventType::SPEEDUPEVENT:
            return QStringLiteral("Speed up");
        case VideoTimedEvent::EventType::AUDIOCOMMENT:
            return QStringLiteral("Audio comment");
        }
        return QString();
    case BeginRole:
        return e->begin();
    case ObjectRole:
        return QVariant::fromValue(e.get());
    case EndRole:
        return e->end();
    case ColorRole:
        return e->color();
    case AudioFileRole:
        if(auto* c= qobject_cast<AudioCommentEvent*>(e.get()))
            return c->audioFile();
        return QString();
    default:
        return {};
    }
}

bool VideoEffectModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if(!index.isValid() || index.row() < 0 || index.row() >= m_data.size())
        return false;

    auto const& e= m_data.at(index.row());
    switch(role)
    {
    case BeginRole:
        e->setBegin(value.toDouble());
        break;
    case EndRole:
        e->setEnd(value.toDouble());
        break;
    default:
        return false;
    }
    emit dataChanged(index, index);
    return true;
}

QHash<int, QByteArray> VideoEffectModel::roleNames() const
{
    return {{Qt::DisplayRole, "display"}, {Id, "rowIndex"}, {TypeRole, "type"},         {TypeNameRole, "typeName"},
            {BeginRole, "begin"},         {EndRole, "end"}, {DurationRole, "duration"}, {ColorRole, "eventColor"},
            {ObjectRole, "effectCtrl"},   {AudioFileRole, "audioFile"}};
}

VideoTimedEvent* VideoEffectModel::effectAt(int row) const
{
    if(row < 0 || row >= m_data.size())
        return nullptr;
    return m_data.at(row).get();
}

VideoTimedEvent* VideoEffectModel::createEvent(VideoTimedEvent::EventType type)
{
    switch(type)
    {
    case VideoTimedEvent::EventType::CUTEVENT:
        return new CutEvent();
    case VideoTimedEvent::EventType::SPEEDUPEVENT:
        return new SpeedUpEvent();
    case VideoTimedEvent::EventType::AUDIOCOMMENT:
        return new AudioCommentEvent();
    }
    return nullptr;
}

void VideoEffectModel::addEffect(VideoTimedEvent::EventType type, qint64 start, qint64 end)
{
    qDebug() << "add effect" << start << end << type;
    VideoTimedEvent* node= createEvent(type);
    if(!node)
        return;
    node->setBegin(start);
    node->setEnd(end);

    beginInsertRows(QModelIndex(), m_data.size(), m_data.size());
    m_data.push_back(std::unique_ptr<VideoTimedEvent>{node});
    endInsertRows();
    connectEvent(node);
    emit effectsChanged();
}

void VideoEffectModel::removeEffect(int row)
{
    if(row < 0 || row >= m_data.size())
        return;
    beginRemoveRows(QModelIndex(), row, row);
    m_data.erase(m_data.begin() + row);
    endRemoveRows();
    emit effectsChanged();
}

void VideoEffectModel::clear()
{
    beginResetModel();
    m_data.clear();
    endResetModel();
    emit effectsChanged();
}

void VideoEffectModel::reset(const QList<VideoTimedEvent*>& infos)
{
    m_data.clear();
    beginResetModel();
    for(auto const& v : infos)
    {
        m_data.push_back(std::unique_ptr<VideoTimedEvent>(v));
    }
    endResetModel();
    for(auto const& v : m_data)
        connectEvent(v.get());
    emit effectsChanged();
}

int VideoEffectModel::rowOf(const VideoTimedEvent* event) const
{
    if(!event)
        return -1;
    auto const it= std::find_if(m_data.begin(), m_data.end(),
                                [event](auto const& e){ return e.get() == event; });
    return it == m_data.end() ? -1 : int(it - m_data.begin());
}

void VideoEffectModel::connectEvent(VideoTimedEvent* event)
{
    if(!event)
        return;
    // The row can change (inserts/removes), so compute it dynamically.
    auto const forward= [this, event]{
        int const row= rowOf(event);
        if(row >= 0)
            emit dataChanged(index(row), index(row));
    };
    QObject::connect(event, &VideoTimedEvent::beginChanged, this, forward);
    QObject::connect(event, &VideoTimedEvent::endChanged, this, forward);

    if(auto* cu= qobject_cast<AudioCommentEvent*>(event))
    {
        QObject::connect(cu, &AudioCommentEvent::textChanged, this, forward);
        QObject::connect(cu, &AudioCommentEvent::audioFileChanged, this, forward);
    }
    else if(auto* su= qobject_cast<SpeedUpEvent*>(event))
    {
        QObject::connect(su, &SpeedUpEvent::timeFactorChanged, this, forward);
    }
}

const std::vector<std::unique_ptr<VideoTimedEvent>>& VideoEffectModel::allEffects() const
{
    return m_data;
}

qreal SpeedUpEvent::timeFactor() const
{
    return m_timeFactor;
}

void SpeedUpEvent::setTimeFactor(qreal newTimeFactor)
{
    if(qFuzzyCompare(m_timeFactor, newTimeFactor))
        return;
    m_timeFactor= newTimeFactor;
    emit timeFactorChanged();
}

QString AudioCommentEvent::text() const
{
    return m_text;
}

void AudioCommentEvent::setText(const QString& newText)
{
    if(m_text == newText)
        return;
    m_text= newText;
    emit textChanged();
}

QString AudioCommentEvent::id() const
{
    return m_id;
}

QString AudioCommentEvent::audioFile() const
{
    return m_audioFile;
}

void AudioCommentEvent::setAudioFile(const QString& newAudioFile)
{
    if(m_audioFile == newAudioFile)
        return;
    m_audioFile= newAudioFile;
    emit audioFileChanged();
}
