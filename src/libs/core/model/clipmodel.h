#ifndef CLIPMODEL_H
#define CLIPMODEL_H

#include "rangemodel.h"
#include <QAbstractListModel>
#include <QColor>
#include <QObject>
#include <QQmlEngine>

class ClipInfo : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(RangeModel* ranges READ ranges CONSTANT)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged FINAL)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged FINAL)
    Q_PROPERTY(QString music READ music WRITE setMusic NOTIFY musicChanged FINAL)
    Q_PROPERTY(QString label READ label WRITE setLabel NOTIFY labelChanged FINAL)
    Q_PROPERTY(qint64 offset READ offset WRITE setOffset NOTIFY offsetChanged FINAL)
    Q_PROPERTY(int duration READ duration NOTIFY durationChanged FINAL)
    Q_PROPERTY(bool selected READ selected WRITE setSelected NOTIFY selectedChanged FINAL)
public:
    explicit ClipInfo(QObject* parent= nullptr);
    ~ClipInfo();

    RangeModel* ranges() const;

    QString name() const;
    void setName(const QString& newName);

    qint64 offset() const;
    void setOffset(qint64 newOffset);

    QString label() const;
    void setLabel(const QString& newLabel);

    QColor color() const;
    void setColor(const QColor& newColor);

    QString music() const;
    void setMusic(const QString& newMusic);

    void addRange(quint64 b, quint64 e);

    int duration() const;
    bool selected() const;
    void setSelected(bool newSelected);

signals:
    void nameChanged();
    void offsetChanged();
    void labelChanged();
    void colorChanged();
    void musicChanged();
    void durationChanged();
    void selectedChanged();
    void rangeAdded();

private:
    QString m_name;
    std::unique_ptr<RangeModel> m_ranges;
    QColor m_color;
    QString m_music;
    qint64 m_offset;
    QString m_label;
    bool m_selected{false};
};

class ClipModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
public:
    enum DataRole
    {
        Name= Qt::UserRole + 1,
        Label,
        Duration,
        Color,
        Music,
        Offset,
        RangeModelRole,
        Selected,
        ClipInfoRole
    };
    Q_ENUM(DataRole)
    ClipModel();

    QVariant data(const QModelIndex& index, int role) const override;
    int rowCount(const QModelIndex& parent= QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    const std::vector<std::unique_ptr<ClipInfo>>& allData() const;

public slots:
    void insertClip(quint64 b, quint64 e);
    void insertClip(ClipInfo* info);
    void rangeIntoCurrentClip(quint64 b, quint64 e);
    void setTitle(int i, const QString& title);
    void reset(const QList<ClipInfo*>& info);
    void clearSelection();

private:
    std::vector<std::unique_ptr<ClipInfo>> m_data;
};

#endif // CLIPMODEL_H
