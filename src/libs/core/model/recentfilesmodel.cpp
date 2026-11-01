#include "recentfilesmodel.h"

#include <QDir>
#include <QUrl>

RecentFilesModel::RecentFilesModel(QObject* parent) : QAbstractListModel(parent) {}

int RecentFilesModel::rowCount(const QModelIndex& parent) const
{
    if(parent.isValid())
        return 0;

    return m_files.size();
}

QVariant RecentFilesModel::data(const QModelIndex& index, int role) const
{
    if(!index.isValid() || index.row() < 0 || index.row() >= m_files.size())
    {
        return {};
    }

    const RecentFile& file= m_files.at(index.row());

    switch(role)
    {
    case FilePathRole:
        return file.filePath;

    case FileNameRole:
        return file.fileName;

    default:
        return {};
    }
}

QHash<int, QByteArray> RecentFilesModel::roleNames() const
{
    return {{FilePathRole, "filePath"}, {FileNameRole, "fileName"}};
}

void RecentFilesModel::addFile(const QString& filePath)
{
    if(filePath.isEmpty())
        return;

    const QString path= filePath;

    const QString name= QUrl(path).fileName();

    if(name.isEmpty())
        return;

    // Remove an existing entry so it can move to the top.
    for(int i= 0; i < m_files.size(); ++i)
    {
        if(m_files.at(i).filePath == path)
        {
            beginRemoveRows(QModelIndex(), i, i);
            m_files.removeAt(i);
            endRemoveRows();
            break;
        }
    }

    // Insert the most recent file at index 0.
    beginInsertRows(QModelIndex(), 0, 0);
    m_files.prepend({path, name});
    endInsertRows();

    // Enforce the maximum number of entries.
    if(m_files.size() > m_maxRecentFiles)
    {
        const int firstRemoved= m_maxRecentFiles;
        const int lastRemoved= m_files.size() - 1;

        beginRemoveRows(QModelIndex(), firstRemoved, lastRemoved);
        m_files.erase(m_files.begin() + firstRemoved, m_files.end());
        endRemoveRows();
    }

    emit countChanged();
}

void RecentFilesModel::removeFile(const QString& filePath)
{
    const QString path= QDir::cleanPath(QFileInfo(filePath).absoluteFilePath());

    for(int i= 0; i < m_files.size(); ++i)
    {
        if(m_files.at(i).filePath == path)
        {
            beginRemoveRows(QModelIndex(), i, i);
            m_files.removeAt(i);
            endRemoveRows();

            emit countChanged();
            return;
        }
    }
}

void RecentFilesModel::clear()
{
    if(m_files.isEmpty())
        return;

    beginResetModel();
    m_files.clear();
    endResetModel();

    emit countChanged();
}

int RecentFilesModel::maxRecentFiles() const
{
    return m_maxRecentFiles;
}

QStringList RecentFilesModel::getData() const
{
    QStringList result;
    result.reserve(m_files.size());

    for(const RecentFile& file : m_files)
        result.append(file.filePath);

    return result;
}

void RecentFilesModel::setData(const QStringList& filePaths)
{
    beginResetModel();
    m_files.clear();

    for(const QString& filePath : filePaths)
    {
        if(filePath.isEmpty())
            continue;

        const QString path= filePath;

        const QString name= QUrl(path).fileName();

        if(name.isEmpty())
            continue;

        // Avoid duplicates.
        bool exists= false;
        for(const RecentFile& file : std::as_const(m_files))
        {
            if(file.filePath == path)
            {
                exists= true;
                break;
            }
        }

        if(exists)
            continue;

        m_files.append({path, name});

        if(m_files.size() >= m_maxRecentFiles)
            break;
    }

    endResetModel();
    emit countChanged();
}

void RecentFilesModel::setMaxRecentFiles(int maximum)
{
    m_maxRecentFiles= qMax(0, maximum);

    if(m_files.size() > m_maxRecentFiles)
    {
        const int firstRemoved= m_maxRecentFiles;
        const int lastRemoved= m_files.size() - 1;

        beginRemoveRows(QModelIndex(), firstRemoved, lastRemoved);
        m_files.erase(m_files.begin() + firstRemoved, m_files.end());
        endRemoveRows();

        emit countChanged();
    }
}
