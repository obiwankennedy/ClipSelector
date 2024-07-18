#ifndef FILTEREDMODEL_H
#define FILTEREDMODEL_H

#include <QSortFilterProxyModel>
#include <QQmlEngine>

class FilteredModel : public QSortFilterProxyModel
{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit FilteredModel(QObject* parent= nullptr);

    QString search() const;
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
    void setSearch(const QString& search);

private:
    QString m_search;
};

#endif // FILTEREDMODEL_H
