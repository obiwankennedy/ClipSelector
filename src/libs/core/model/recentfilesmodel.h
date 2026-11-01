#ifndef RECENTFILESMODEL_H
#define RECENTFILESMODEL_H

#include <QAbstractItemModel>

#include <QAbstractListModel>
#include <QStringList>

class RecentFilesModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles
    {
        FilePathRole= Qt::UserRole + 1,
        FileNameRole
    };
    Q_ENUM(Roles)

    explicit RecentFilesModel(QObject* parent= nullptr);

    int rowCount(const QModelIndex& parent= QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role= Qt::DisplayRole) const override;

    QHash<int, QByteArray> roleNames() const override;

    int maxRecentFiles() const;
    void setMaxRecentFiles(int maximum);

public slots:
    void addFile(const QString& filePath);
    void removeFile(const QString& filePath);
    void clear();
    QStringList getData() const;
    void setData(const QStringList& filePaths);

signals:
    void countChanged();

private:
    struct RecentFile
    {
        QString filePath;
        QString fileName;
    };

    QList<RecentFile> m_files;
    int m_maxRecentFiles= 10;
};

#endif // RECENTFILESMODEL_H
