#include "clipmodel.h"

ClipModel::ClipModel() {}

QVariant ClipModel::data(const QModelIndex& index, int role) const
{
    if(!index.isValid())
        return {};

    auto& clip = m_data[index.row()];

    auto c = index.column();

    auto vrole = Label+c;

    if(role < Label)
    {
        role = vrole;
        qDebug() << "vrole different than role" << vrole << role;
    }

    QVariant res;
    switch(role)
    {
    case Label:
        res = clip.label;
        break;
    case Begin:
        res = clip.begin;
        break;
    case End:
        res = clip.end;
        break;
    case Duration:
        res = (clip.end-clip.begin)/1000;
        break;
    case Music:
        qDebug() << clip.music;
        res = clip.music;
        break;
    case Offset:
        res = clip.offset;
        break;
    }
    return res;
}

Qt::ItemFlags ClipModel::flags(const QModelIndex& index)const
{
    Q_UNUSED(index)
    return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool ClipModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if(!index.isValid())
        return false;

    auto& clip = m_data[index.row()];

    switch(role)
    {
    case Label:
        clip.label = value.toString();
        break;
    case Begin:
        clip.begin = value.toULongLong();
        break;
    case End:
        clip.end = value.toULongLong();
        break;
    case Music:
        clip.music = value.toString();
        break;
    case Offset:
        clip.offset = value.toULongLong();
        break;
    case Duration:
        break;
    }
    qDebug() << "datachanged" << index << index.siblingAtColumn(columnCount(QModelIndex())-1) << Label << Music;
    emit dataChanged(index, index.siblingAtColumn(columnCount(QModelIndex())-1), {role});
    return true;
}

int ClipModel::rowCount(const QModelIndex& idx) const
{
    if(idx.isValid())
        return 0;

    return m_data.size();
}

int ClipModel::columnCount(const QModelIndex &idx) const
{
    if(idx.isValid())
        return 0;

    return 6;
}

QHash<int, QByteArray> ClipModel::roleNames() const
{
    return {{Duration,"duration"},{Label,"label"},{Begin, "begin"}, {End, "end"},{Music, "music"},{Offset, "offset"}};
}

const std::vector<ClipInfo> &ClipModel::allData() const
{
    return m_data;
}

void ClipModel::insertClip(quint64 b, quint64 e)
{
    auto it = std::find_if(std::begin(m_data), std::end(m_data), [b](const ClipInfo& info){
        return b < info.begin;
    });

    auto pos = m_data.size();
    if(it != std::end(m_data))
        pos = std::distance(std::begin(m_data), it);


    beginInsertRows(QModelIndex(), pos, pos);
    m_data.insert(it, ClipInfo{tr("Untitled"),b,e});
    endInsertRows();
}

void ClipModel::setTitle(int i, const QString& title)
{
    auto& clip = m_data[i];
    clip.label = title;
    emit dataChanged(index(i, 0),index(i, 0), {Label});
}

void ClipModel::reset()
{
    beginResetModel();
    m_data.clear();
    endResetModel();
}
