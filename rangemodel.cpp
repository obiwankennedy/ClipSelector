#include "rangemodel.h"

int computeDuration(const std::pair<quint64, quint64>& info)
{
    return (info.second - info.first) / 1000;
}

int cumulateDuration(const QList<std::pair<quint64, quint64>>& ranges)
{
    return std::accumulate(std::begin(ranges), std::end(ranges), 0,
                           [](int a, const std::pair<quint64, quint64>& b) { return a + computeDuration(b); });
}

RangeModel::RangeModel(QObject* parent) : QAbstractListModel(parent) {}

int RangeModel::rowCount(const QModelIndex& parent) const
{
    if(parent.isValid())
        return 0;

    return m_ranges.size();
}

QVariant RangeModel::data(const QModelIndex& index, int role) const
{
    if(!index.isValid())
        return QVariant();

    auto const& i= m_ranges.at(index.row());
    QVariant res;
    switch(role)
    {
    case Begin:
        res= i.first;
        break;
    case End:
        res= i.second;
        break;
    case Duration:
        res= computeDuration(i);
        break;
    }
    return res;
}

bool RangeModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    qDebug() << "setData RangeModel" << index << value << role;
    if(!index.isValid())
        return false;

    auto& i= m_ranges[index.row()];
    switch(role)
    {
    case Begin:
        i.first= value.toULongLong();
        break;
    case End:
        i.second= value.toULongLong();
        break;
    }
    emit dataChanged(index, index);
    return true;
}

QHash<int, QByteArray> RangeModel::roleNames() const
{
    return {{Begin, "begin"}, {End, "end"}, {Duration, "duration"}};
}

const QList<std::pair<quint64, quint64>>& RangeModel::allRanges() const
{
    return m_ranges;
}

void RangeModel::addRange(quint64 b, quint64 e)
{
    beginInsertRows(QModelIndex(), m_ranges.size(), m_ranges.size());
    m_ranges.append({b, e});
    endInsertRows();
}

int RangeModel::duration() const
{
    return cumulateDuration(m_ranges);
}
