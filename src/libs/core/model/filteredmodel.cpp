#include "filteredmodel.h"

#include "audiofilemodel.h"

FilteredModel::FilteredModel(QObject* parent) : QSortFilterProxyModel(parent) {}

QString FilteredModel::search() const
{
    return m_search;
}

bool FilteredModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    if(m_search.isEmpty())
        return true;
    QModelIndex index= sourceModel()->index(sourceRow, 0, sourceParent);
    auto title= index.data(AudioFileModel::TitleRole).toString();
    auto artist= index.data(AudioFileModel::ArtistRole).toString();
    return title.contains(m_search, Qt::CaseInsensitive) || artist.contains(m_search, Qt::CaseInsensitive);
}

void FilteredModel::setSearch(const QString& search)
{
    m_search= search;
    invalidateFilter();
}

ClipListModel::ClipListModel() {}

namespace
{
    // Maps a flat ClipListModel row to the ClipInfo it belongs to and the
    // index of the range inside that clip.
    struct RowMapping
    {
        ClipInfo* info= nullptr;
        int rangeIndex= 0;
    };

    RowMapping resolveRow(const std::vector<std::unique_ptr<ClipInfo>>& infos, int row)
    {
        RowMapping m;
        int acc= 0;
        for(auto const& info : infos)
        {
            auto size= info->ranges()->rowCount();
            if(row < acc + size)
            {
                m.info= info.get();
                m.rangeIndex= row - acc;
                break;
            }
            acc+= size;
        }
        return m;
    }
}

QVariant ClipListModel::data(const QModelIndex& index, int role) const
{
    if(!m_model)
        return {};
    auto mapping= resolveRow(m_model->allData(), index.row());
    if(!mapping.info)
        return {};
    auto ranges= mapping.info->ranges();
    QVariant res;
    switch(role)
    {
    case beginRole:
        res= ranges->data(ranges->index(mapping.rangeIndex, 0), RangeModel::Begin);
        break;
    case endRole:
        res= ranges->data(ranges->index(mapping.rangeIndex, 0), RangeModel::End);
        break;
    case colorRole:
        res= QVariant::fromValue(mapping.info->color());
        break;
    case labelRole:
        res= mapping.info->label();
        break;
    }

    return res;
}

Qt::ItemFlags ClipListModel::flags(const QModelIndex& index) const
{
    if(!index.isValid())
        return Qt::NoItemFlags;
    return Qt::ItemIsEditable | Qt::ItemIsEnabled;
}

bool ClipListModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if(!index.isValid() || !m_model)
        return false;
    auto mapping= resolveRow(m_model->allData(), index.row());
    if(!mapping.info)
        return false;
    auto ranges= mapping.info->ranges();
    auto rIndex= ranges->index(mapping.rangeIndex, 0);
    bool ok= false;
    switch(role)
    {
    case beginRole:
        ok= ranges->setData(rIndex, value, RangeModel::Begin);
        break;
    case endRole:
        ok= ranges->setData(rIndex, value, RangeModel::End);
        break;
    default:
        return false;
    }
    if(!ok)
        return false;
    // The RangeModel's dataChanged is not observed by this model, so notify
    // our own QML delegate that this row changed.
    emit dataChanged(index, index);
    return true;
}

int ClipListModel::rowCount(const QModelIndex& parent) const
{
    if(parent.isValid() || !m_model)
        return 0;
    auto const& infos= m_model->allData();

    return std::accumulate(std::begin(infos), std::end(infos), 0,
                           [](int a, const std::unique_ptr<ClipInfo>& info) { return a + info->ranges()->rowCount(); });
}

QHash<int, QByteArray> ClipListModel::roleNames() const
{
    return {{beginRole, "begin"}, {endRole, "end"}, {colorRole, "clipColor"}, {labelRole, "label"}};
}

ClipModel* ClipListModel::sourceModel() const
{
    return m_model;
}

void ClipListModel::setSourceModel(ClipModel* newSourceModel)
{
    if(m_model == newSourceModel)
        return;
    m_model= newSourceModel;
    emit sourceModelChanged();

    connect(m_model, &ClipModel::rowsInserted, this,
            [this]()
            {
                beginResetModel();
                endResetModel();
            });
    connect(m_model, &ClipModel::rowsRemoved, this,
            [this]()
            {
                beginResetModel();
                endResetModel();
            });
    connect(m_model, &ClipModel::modelReset, this,
            [this]()
            {
                beginResetModel();
                endResetModel();
            });
    connect(m_model, &ClipModel::dataChanged, this,
            [this]()
            {
                beginResetModel();
                endResetModel();
            });
}
