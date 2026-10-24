#ifndef RANGEMODEL_H
#define RANGEMODEL_H

#include <QAbstractListModel>
#include <QObject>
#include <QQmlEngine>

class RangeModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum CustomRole
    {
        Begin,
        End,
        Duration
    };
    Q_ENUM(CustomRole)
    explicit RangeModel(QObject* parent= nullptr);

    int rowCount(const QModelIndex& parent= QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role= Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    QHash<int, QByteArray> roleNames() const override;

    const QList<std::pair<quint64, quint64>>& allRanges() const;

    void addRange(quint64 b, quint64 e);

    int duration() const;

private:
    QList<std::pair<quint64, quint64>> m_ranges;
};

#endif // RANGEMODEL_H
