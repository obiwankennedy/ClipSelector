#include "clipmodel.h"

#include <chrono>
#include <numeric>
#include <random>

QColor generateColor()
{
    static std::mt19937_64 mt(std::chrono::system_clock::now().time_since_epoch().count());
    static std::uniform_int_distribution<int> redAndBlueDist(0, 255);
    static std::uniform_int_distribution<int> greenDist(0, 128);
    return QColor(redAndBlueDist(mt), greenDist(mt), redAndBlueDist(mt));
}

/*int computeDuration(const RangeInfo& info)
{
    return (info.end - info.begin) / 1000;
}

int cumulateDuration(const std::vector<RangeInfo>& ranges)
{
    return std::accumulate(std::begin(ranges), std::end(ranges), 0,
                           [](int a, const RangeInfo& b) { return a + computeDuration(b); });
}*/

ClipModel::ClipModel() {}

QVariant ClipModel::data(const QModelIndex& index, int role) const
{
    if(!index.isValid())
        return {};

    static QSet<int> allowed{Qt::DisplayRole, Duration, Color, Music, Offset, Name, Label, RangeModelRole};

    if(!allowed.contains(role))
        return {};

    auto id= static_cast<int>(index.internalId()) - 1;

    int row= index.row();
    auto clip= m_data[row].get();

    QVariant res;
    switch(role)
    {
    case Label:
        res= clip->label();
        break;
    case Name:
        res= clip->name();
        break;
    case RangeModelRole:
        res= QVariant::fromValue(clip->ranges());
        break;
    case Duration:
        res= clip->duration();
        break;
    case Music:
        res= clip->music();
        break;
    case Color:
        res= clip->color();
        break;
    case Offset:
        res= clip->offset();
        break;
    }
    // qDebug() << "data index:" << index << role << res;
    return res;
}

Qt::ItemFlags ClipModel::flags(const QModelIndex& index) const
{
    if(!index.isValid())
        return Qt::NoItemFlags;
    return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool ClipModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    qDebug() << "setData clipmode" << index << value << role;
    if(!index.isValid())
        return false;

    auto id= static_cast<int>(index.internalId()) - 1;

    int row= index.row();

    auto clip= m_data[row].get();
    // auto ranges= clip->ranges();

    switch(role)
    {
    case Label:
        clip->setLabel(value.toString());
        break;
    case Name:
        clip->setName(value.toString());
        break;
    case Music:
        clip->setMusic(value.toString());
        break;
    case Color:
        clip->setColor(value.toString());
        break;
    case Offset:
        clip->setOffset(value.toLongLong());
        break;
    case Duration:
        break;
    }
    emit dataChanged(index, index);
    return true;
}

int ClipModel::rowCount(const QModelIndex& parent) const
{
    if(parent.isValid())
        return 0;

    return m_data.size();

    /* auto res= 0;
     auto id= static_cast<TreeId*>(parent.internalPointer());
     if(!parent.isValid())
         res= m_data.size();
     else if(id)
     {
         if(!id->range)
         {
             auto clip= m_data.at(id->row);
             res= clip.ranges.size();
         }
     }
     else if(id == 0)
     {
         qDebug() << "count title";
         auto clip= m_data.at(id);
         res= clip.ranges.size();
     }
     qDebug() << "parent" << parent << "rowCount" << res << id;*/
    // return res;
}

/*QModelIndex ClipModel::index(int row, int column, const QModelIndex& parent) const
{
    if(!parent.isValid())
        return createIndex(row, column, static_cast<quintptr>(0));

    // auto gParent= parent.parent();
    // if(gParent.isValid())
    // return createIndex(row, column, static_cast<quintptr>(parent.row() + 1));
    // else
    return createIndex(row, column, static_cast<quintptr>(parent.row() + 1));
}

QModelIndex ClipModel::parent(const QModelIndex& child) const
{
    auto id= static_cast<int>(child.internalId() - 1);

    if(id < 0)
        return QModelIndex();

    return createIndex(id, 0, static_cast<quintptr>(0));
}*/

QHash<int, QByteArray> ClipModel::roleNames() const
{
    return {{Duration, "duration"}, {Color, "clipColor"},       {0, "display"},  {Music, "music"},
            {Offset, "offset"},     {RangeModelRole, "ranges"}, {Label, "label"}};
}

const std::vector<std::unique_ptr<ClipInfo>>& ClipModel::allData() const
{
    return m_data;
}

void ClipModel::insertClip(quint64 b, quint64 e)
{
    auto it= std::find_if(std::begin(m_data), std::end(m_data),
                          [b](const std::unique_ptr<ClipInfo>& node)
                          {
                              auto model= node->ranges();

                              Q_ASSERT(node->ranges()->rowCount() > 0);
                              auto it= model->data(model->index(0, 0, QModelIndex()), RangeModel::Begin).toULongLong();
                              return b < it;
                          });

    auto pos= m_data.size();
    if(it != std::end(m_data))
        pos= std::distance(std::begin(m_data), it);

    beginInsertRows(QModelIndex(), pos, pos);
    std::unique_ptr<ClipInfo> node{new ClipInfo};
    node->setName(tr("Untitled"));
    node->addRange(b, e);
    node->setColor(generateColor());
    m_data.insert(it, std::move(node));
    endInsertRows();
}

void ClipModel::rangeIntoCurrentClip(quint64 b, quint64 e)
{
    if(m_data.empty())
        return;

    auto clip= m_data[m_data.size() - 1].get();
    clip->addRange(b, e);
}

void ClipModel::insertClip(ClipInfo* info)
{
    beginInsertRows(QModelIndex(), m_data.size(), m_data.size());
    std::unique_ptr<ClipInfo> p(info);
    m_data.push_back(std::move(p));
    endInsertRows();
}

void ClipModel::setTitle(int i, const QString& title)
{
    auto clip= m_data[i].get();
    clip->setLabel(title);
    emit dataChanged(index(i, 0), index(i, 0), {Label});
}

void ClipModel::reset(const QList<ClipInfo*>& infos)
{
    beginResetModel();
    m_data.clear();
    for(auto const& info : infos)
    {
        std::unique_ptr<ClipInfo> p(info);
        m_data.push_back(std::move(p));
    }
    endResetModel();
}

ClipInfo::ClipInfo(QObject* parent) : m_ranges{new RangeModel}
{
    qDebug() << "Constructor ClipInfo";
}

ClipInfo::~ClipInfo()
{
    qDebug() << "Destructor ClipInfo";
} //-8.888s

RangeModel* ClipInfo::ranges() const
{
    return m_ranges.get();
}

QString ClipInfo::name() const
{
    return m_name;
}

void ClipInfo::setName(const QString& newName)
{
    if(m_name == newName)
        return;
    m_name= newName;
    emit nameChanged();
}

qint64 ClipInfo::offset() const
{
    return m_offset;
}

void ClipInfo::setOffset(qint64 newOffset)
{
    if(m_offset == newOffset)
        return;
    m_offset= newOffset;
    emit offsetChanged();
}

QString ClipInfo::label() const
{
    return m_label;
}

void ClipInfo::setLabel(const QString& newLabel)
{
    if(m_label == newLabel)
        return;
    m_label= newLabel;
    emit labelChanged();
}

QColor ClipInfo::color() const
{
    return m_color;
}

void ClipInfo::setColor(const QColor& newColor)
{
    if(m_color == newColor)
        return;
    m_color= newColor;
    emit colorChanged();
}

QString ClipInfo::music() const
{
    return m_music;
}

void ClipInfo::setMusic(const QString& newMusic)
{
    if(m_music == newMusic)
        return;
    m_music= newMusic;
    emit musicChanged();
}

void ClipInfo::addRange(quint64 b, quint64 e)
{
    m_ranges->addRange(b, e);
}

int ClipInfo::duration() const
{
    return m_ranges->duration();
}
