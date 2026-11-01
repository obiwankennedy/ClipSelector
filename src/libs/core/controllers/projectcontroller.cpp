#include "projectcontroller.h"

#include <QFileInfo>
#include <QSaveFile>
#include <QSettings>

ProjectController::ProjectController(QObject* parent)
    : QObject{parent}, m_recentVideos{new RecentFilesModel}, m_recentProjects{new RecentFilesModel}
{
}

void ProjectController::openVideo(const QString& path)
{
    qDebug() << "openVideo " << path;
    // Derive the project path from the video path and set it BEFORE the video
    // path: MainController::reset() (triggered by videoFilePathChanged) needs
    // the project path to be in place to (re)load the matching .json file.
    setProjectFilePath(deriveProjectPath(path));
    setVideoFilePath(path);

    addRecentVideo(path);
}

void ProjectController::openProject(const QString& path)
{
    qDebug() << "openProject " << path;
    addRecentProject(path);
}

void ProjectController::saveProject(const QString& path)
{
    auto filePath= QUrl(path).toLocalFile();
    if(!filePath.endsWith(".json"))
        filePath+= ".json";

    QSaveFile file(filePath);
    if(file.open(QIODevice::WriteOnly))
    {
        file.commit();
        setProjectFilePath(filePath);
        addRecentProject(filePath);
    }
    else
    {
        qDebug() << "Failed to save project" << filePath << file.errorString();
    }
}
void ProjectController::loadSettings()
{
    QSettings settings("rolisteam", "ClipSelector");
    qDebug() << "loadSettings:" << settings.value("recentvideos").toStringList();
    qDebug() << "loadSettings:" << settings.value("recentprojects").toStringList();
    m_recentVideos->setData(settings.value("recentvideos").toStringList());
    m_recentProjects->setData(settings.value("recentprojects").toStringList());
}
void ProjectController::writeSettings() const
{
    QSettings settings("rolisteam", "ClipSelector");
    qDebug() << "writeSettings:" << m_recentVideos->getData();
    qDebug() << "writeSettings:" << m_recentProjects->getData();
    settings.setValue("recentvideos", m_recentVideos->getData());
    settings.setValue("recentprojects", m_recentProjects->getData());
}

RecentFilesModel* ProjectController::recentVideos() const
{
    return m_recentVideos.get();
}

RecentFilesModel* ProjectController::recentProjects() const
{
    return m_recentProjects.get();
}

ProjectController::ProjectType ProjectController::projectType() const
{
    return m_projectType;
}

void ProjectController::setProjectType(ProjectController::ProjectType newProjectType)
{
    if(m_projectType == newProjectType)
        return;
    m_projectType= newProjectType;
    emit projectTypeChanged();
}

QStringList ProjectController::projectTypeNames()
{
    return {QString::fromUtf8("MUSIC CLIP"), QString::fromUtf8("TUTORIAL")};
}

QString ProjectController::projectTypeToString(ProjectType type)
{
    return type == Tutorial ? "tutorial" : "musicclip";
}

ProjectController::ProjectType ProjectController::projectTypeFromString(const QString& s)
{
    return s == "tutorial" ? Tutorial : MusicClip;
}
QString ProjectController::videoFilePath() const
{
    return m_videoFilePath;
}

void ProjectController::setVideoFilePath(const QString& newVideoFilePath)
{
    if(m_videoFilePath == newVideoFilePath)
        return;
    m_videoFilePath= newVideoFilePath;
    emit videoFilePathChanged();
}

QString ProjectController::projectFilePath() const
{
    return m_projectFilePath;
}
void ProjectController::clearRecentVideos()
{
    m_recentVideos->clear();
}

QString ProjectController::deriveProjectPath(const QString& videoPath)
{
    QFileInfo fi(videoPath);
    if(fi.suffix().isEmpty())
        return fi.absoluteFilePath() + ".json";
    return fi.absolutePath() + "/" + fi.completeBaseName() + ".json";
}

void ProjectController::addRecentVideo(const QString& path)
{
    m_recentVideos->addFile(path);
}

void ProjectController::addRecentProject(const QString& path)
{
    m_recentProjects->addFile(path);
}

void ProjectController::clearRecentProjects()
{
    m_recentProjects->clear();
}
void ProjectController::setProjectFilePath(const QString& newProjectFilePath)
{
    if(m_projectFilePath == newProjectFilePath)
        return;
    m_projectFilePath= newProjectFilePath;
    emit projectFilePathChanged();
}
