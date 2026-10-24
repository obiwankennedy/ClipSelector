#ifndef FILTEREDMODEL_H
#define FILTEREDMODEL_H

#include "clipmodel.h"
#include <QQmlEngine>
#include <QSortFilterProxyModel>

class ClipListModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(ClipModel* sourceModel READ sourceModel WRITE setSourceModel NOTIFY sourceModelChanged FINAL)

public:
    enum Roles
    {
        beginRole= Qt::UserRole + 1,
        endRole,
        colorRole,
        labelRole
    };
    ClipListModel();

    QVariant data(const QModelIndex& index, int role) const override;
    int rowCount(const QModelIndex& parent) const override;
    QHash<int, QByteArray> roleNames() const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    ClipModel* sourceModel() const;
    void setSourceModel(ClipModel* newSourceModel);

signals:
    void sourceModelChanged();

private:
    QPointer<ClipModel> m_model;
};

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
