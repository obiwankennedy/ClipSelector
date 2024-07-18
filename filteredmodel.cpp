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
