#include "clipmodel.h"

#include <chrono>
#include <random>

QColor generateColor()
{
    static std::mt19937_64 mt(std::chrono::system_clock::now().time_since_epoch().count());
    static std::uniform_int_distribution<int> redAndBlueDist(0, 255);
    static std::uniform_int_distribution<int> greenDist(0, 128);
    return QColor(redAndBlueDist(mt), greenDist(mt), redAndBlueDist(mt));
}

ClipModel::ClipModel() {}

QVariant ClipModel::data(const QModelIndex& index, int role) const
{
    if(!index.isValid())
        return {};

    auto& clip= m_data[index.row()];

    auto c= index.column();

    auto vrole= Label + c;

    if(role < Label)
    {
        role= vrole;
        qDebug() << "vrole different than role" << vrole << role;
    }

    QVariant res;
    switch(role)
    {
    case Label:
        res= clip.label;
        break;
    case Ranges:
        res= QVariant::fromValue(clip.ranges);
        break;
    case Duration:
        res= clip.ranges->duration();
        break;
    case Music:
        qDebug() << clip.music;
        res= clip.music;
        break;
    case Color:
        res= clip.color;
        break;
    case Offset:
        res= clip.offset;
        break;
    }
    return res;
}

Qt::ItemFlags ClipModel::flags(const QModelIndex& index) const
{
    Q_UNUSED(index)
    return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool ClipModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if(!index.isValid())
        return false;

    auto& clip= m_data[index.row()];

    switch(role)
    {
    case Label:
        clip.label= value.toString();
        break;
    case Ranges:
        clip.ranges= value.value<RangeModel*>();
        break;
    case Music:
        clip.music= value.toString();
        break;
    case Offset:
        clip.offset= value.toLongLong();
        break;
    case Duration:
        break;
    }
    qDebug() << "datachanged" << index << index.siblingAtColumn(columnCount(QModelIndex()) - 1) << Label << Music;
    emit dataChanged(index, index.siblingAtColumn(columnCount(QModelIndex()) - 1), {role});
    return true;
}

int ClipModel::rowCount(const QModelIndex& idx) const
{
    if(idx.isValid())
        return 0;

    return m_data.size();
}

int ClipModel::columnCount(const QModelIndex& idx) const
{
    if(idx.isValid())
        return 0;

    return 5;
}

QHash<int, QByteArray> ClipModel::roleNames() const
{
    return {{Duration, "duration"}, {Label, "label"}, {Color, "clipColor"},
            {Ranges, "ranges"},     {Music, "music"}, {Offset, "offset"}};
}

const std::vector<ClipInfo>& ClipModel::allData() const
{
    return m_data;
}

void ClipModel::insertClip(quint64 b, quint64 e)
{
    auto it= std::find_if(std::begin(m_data), std::end(m_data),
                          [b](const ClipInfo& info)
                          {
                              auto it= info.ranges->begin();
                              return b < it;
                          });

    auto pos= m_data.size();
    if(it != std::end(m_data))
        pos= std::distance(std::begin(m_data), it);

    beginInsertRows(QModelIndex(), pos, pos);

    auto model= new RangeModel;
    connect(model, &RangeModel::dataChanged, this,
            [this]()
            {
                auto model= qobject_cast<RangeModel*>(sender());
                auto it= std::find_if(std::begin(m_data), std::end(m_data),
                                      [model](const ClipInfo& info) { return info.ranges == model; });
                if(it == std::end(m_data))
                    return;
                auto dist= std::distance(std::begin(m_data), it);

                emit dataChanged(index(dist, 0), {});
            });
    model->addRange(b, e);
    m_data.insert(it, ClipInfo{tr("Untitled"), model, generateColor()});
    endInsertRows();
}

void ClipModel::rangeIntoCurrentClip(quint64 b, quint64 e)
{
    if(m_data.empty())
        return;

    auto clip= m_data[m_data.size() - 1];
    clip.ranges->addRange(b, e);
}

void ClipModel::insertClip(const ClipInfo& info)
{
    beginInsertRows(QModelIndex(), m_data.size(), m_data.size());
    m_data.push_back(info);
    endInsertRows();
}

void ClipModel::setTitle(int i, const QString& title)
{
    auto& clip= m_data[i];
    clip.label= title;
    emit dataChanged(index(i, 0), index(i, 0), {Label});
}

void ClipModel::reset()
{
    beginResetModel();
    m_data.clear();
    endResetModel();
}

RangeModel::RangeModel() {}

QVariant RangeModel::data(const QModelIndex& index, int role) const
{
    if(!index.isValid())
        return {};

    auto& range= m_data[index.row()];

    auto c= index.column();

    auto vrole= Begin + c;

    if(role < Begin)
    {
        role= vrole;
        qDebug() << "vrole different than role" << vrole << role;
    }

    QVariant res;
    switch(role)
    {
    case Begin:
        res= range.begin;
        break;
    case End:
        res= range.end;
        break;
    }

    return res;
}

int RangeModel::rowCount(const QModelIndex& index) const
{
    if(index.isValid())
        return 0;
    return m_data.size();
}

QHash<int, QByteArray> RangeModel::roleNames() const
{
    return {{Begin, "begin"}, {End, "end"}};
}

bool RangeModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if(!index.isValid())
        return false;

    auto& info= m_data[index.row()];

    switch(role)
    {
    case Begin:
        info.begin= value.toInt();
        break;
    case End:
        info.end= value.toInt();
        break;
    }
    emit dataChanged(index, index, {role});
    return true;
}

Qt::ItemFlags RangeModel::flags(const QModelIndex& index) const
{
    Q_UNUSED(index);
    return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

int RangeModel::duration() const
{
    return std::accumulate(std::begin(m_data), std::end(m_data), 0,
                           [](int a, const RangeInfo& info) { return a + ((info.end - info.begin) / 1000); });
}

void RangeModel::addRange(quint64 b, quint64 e)
{
    beginInsertRows(QModelIndex(), m_data.size(), m_data.size());
    m_data.push_back(RangeInfo{b, e});
    endInsertRows();

    emit durationChanged();
}

quint64 RangeModel::begin() const
{
    if(m_data.empty())
        return 0;

    return m_data[0].begin;
}

quint64 RangeModel::end() const
{
    if(m_data.empty())
        return 0;

    return m_data[m_data.size() - 1].end;
}

const std::vector<RangeInfo> RangeModel::ranges() const
{
    return m_data;
}

void RangeModel::setRanges(const std::vector<RangeInfo>& infos)
{
    beginResetModel();
    m_data= infos;
    endResetModel();
}

quint64 RangeModel::closiestPositionInClip(quint64 pos, bool* ok)
{
    if(ok)
        *ok= true;
    for(auto r : m_data)
    {
        if(pos <= r.begin)
        {
            pos= r.begin;
            return pos;
        }

        if(pos > r.begin && pos < r.end)
            return pos;

        if(pos > r.begin)
            continue;
    }

    if(ok)
        *ok= false;
    return pos;
}
