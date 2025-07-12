#ifndef FILTEREDMODEL_H
#define FILTEREDMODEL_H

#include <QQmlEngine>
#include <QSortFilterProxyModel>

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

    // aunaur
};

#endif // FILTEREDMODEL_H
