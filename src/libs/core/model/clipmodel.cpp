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
    case Selected:
        res= clip->selected();
        break;
    case ClipInfoRole:
        res= QVariant::fromValue(clip);
        break;
    case Offset:
        res= clip->offset();
        break;
    }
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
    case Selected:
        clip->setSelected(value.toBool());
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
}

QHash<int, QByteArray> ClipModel::roleNames() const
{
    return {{Duration, "duration"}, {Color, "clipColor"},       {0, "display"},
            {Music, "music"},       {Offset, "offset"},         {RangeModelRole, "ranges"},
            {Label, "label"},       {ClipInfoRole, "clipInfo"}, {Selected, "selected"}};
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
    qDebug() << "rangeInto Current clip" << m_data.empty() << b << e;
    if(m_data.empty())
        return;

    auto idx= m_data.size() - 1;
    auto clip= m_data[idx].get();
    clip->addRange(b, e);
    emit dataChanged(index(idx, 0), index(idx, 0), {RangeModelRole});
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

void ClipModel::reset(const QList<ClipInfo*>& info)
{
    beginResetModel();
    m_data.clear();
    for(auto const& info : info)
    {
        std::unique_ptr<ClipInfo> p(info);
        m_data.push_back(std::move(p));
    }
    endResetModel();
}

void ClipModel::clearSelection()
{
    for(auto const& p : m_data)
    {
        p->setSelected(false);
    }
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
    emit rangeAdded();
}

int ClipInfo::duration() const
{
    return m_ranges->duration();
}

bool ClipInfo::selected() const
{
    return m_selected;
}

void ClipInfo::setSelected(bool newSelected)
{
    if(m_selected == newSelected)
        return;
    m_selected= newSelected;
    emit selectedChanged();
}
