#ifndef CLIPMODEL_H
#define CLIPMODEL_H

#include <QAbstractListModel>
#include <QColor>
#include <QObject>
#include <QQmlEngine>

struct RangeInfo
{
    quint64 begin;
    quint64 end;
};

class RangeModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int duration READ duration NOTIFY durationChanged FINAL)
public:
    enum DataRole
    {
        Begin= Qt::UserRole + 1,
        End
    };
    Q_ENUM(DataRole)

    RangeModel();

    QVariant data(const QModelIndex& index, int role) const override;
    int rowCount(const QModelIndex& index) const override;
    QHash<int, QByteArray> roleNames() const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    int duration() const;
    quint64 begin() const;
    quint64 end() const;

    const std::vector<RangeInfo> ranges() const;
    void setRanges(const std::vector<RangeInfo>& infos);

public slots:
    quint64 closiestPositionInClip(quint64 pos, bool* ok= nullptr);
    void addRange(quint64 b, quint64 e);

signals:
    void durationChanged();

private:
    std::vector<RangeInfo> m_data;
    int m_duration;
};

struct ClipInfo
{
    QString label;
    RangeModel* ranges;
    QColor color;
    QString music;
    qint64 offset;
};

class ClipModel : public QAbstractTableModel
{
    Q_OBJECT
    QML_ELEMENT
public:
    enum DataRole
    {
        Label= Qt::UserRole + 1,
        Ranges,
        Duration,
        Color,
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
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    const std::vector<ClipInfo>& allData() const;

public slots:
    void insertClip(quint64 b, quint64 e);
    void insertClip(const ClipInfo& info);
    void rangeIntoCurrentClip(quint64 b, quint64 e);
    void setTitle(int i, const QString& title);
    void reset();

private:
    std::vector<ClipInfo> m_data;
};

#endif // CLIPMODEL_H
