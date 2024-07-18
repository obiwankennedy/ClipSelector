#ifndef CLIPMODEL_H
#define CLIPMODEL_H

#include <QAbstractListModel>
#include <QObject>
#include <QQmlEngine>

struct ClipInfo {
    QString label;
    quint64 begin;
    quint64 end;
    QString music;
    quint64 offset;
};

class ClipModel : public QAbstractTableModel
{
    Q_OBJECT
    QML_ELEMENT
public:
    enum DataRole {
        Label = Qt::UserRole+1,
        Begin,
        End,
        Duration,
        Music,
        Offset
    };
    Q_ENUM(DataRole)
    ClipModel();

    QVariant data(const QModelIndex& index, int role) const override;
    int rowCount(const QModelIndex& idx) const override;
    int columnCount(const QModelIndex& idx) const override;
    QHash<int, QByteArray> roleNames() const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    Qt::ItemFlags flags(const QModelIndex& index)const override;

    const std::vector<ClipInfo>& allData() const;

public slots:
    void insertClip(quint64 b, quint64 e);
    void setTitle(int i, const QString& title);
    void reset();
private:
    std::vector<ClipInfo> m_data;
};

#endif // CLIPMODEL_H
