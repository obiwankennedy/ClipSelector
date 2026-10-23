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

QVariant ClipListModel::data(const QModelIndex& index, int role) const
{
    auto r= index.row();
    int rangeIndex= 0;
    auto const& infos= m_model->allData();
    int i= 0;
    while(r > 0 && infos.size() > i)
    {
        auto size= infos[i]->ranges()->rowCount();
        if(r - size >= 0)
            r-= size;
        else
        {
            rangeIndex= r;
            r-= size;
        }
        ++i;
    }
    auto info= infos[i].get();
    if(!info)
        return {};
    auto ranges= info->ranges();
    QVariant res;
    switch(role)
    {
    case beginRole:
        res= ranges->data(ranges->index(rangeIndex, 0), RangeModel::Begin);
        break;
    case endRole:
        res= ranges->data(ranges->index(rangeIndex, 0), RangeModel::End);
        break;
    case colorRole:
        res= QVariant::fromValue(info->color());
        break;
    case labelRole:
        res= info->label();
        break;
    }

    return res;
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
