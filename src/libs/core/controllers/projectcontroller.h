#ifndef PROJECTCONTROLLER_H
#define PROJECTCONTROLLER_H

#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <memory>

#include "recentfilesmodel.h"

// Central controller for the current project: owns the source video path,
// the project file path (auto-derived from the video path, renameable),
// and the models of recently opened videos/projects.
class ProjectController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    // clang-format off
    Q_PROPERTY(QString videoFilePath READ videoFilePath WRITE setVideoFilePath NOTIFY videoFilePathChanged FINAL)
    Q_PROPERTY(QString projectFilePath READ projectFilePath WRITE setProjectFilePath NOTIFY projectFilePathChanged FINAL)
    Q_PROPERTY(RecentFilesModel* recentVideos READ recentVideos CONSTANT FINAL)
    Q_PROPERTY(RecentFilesModel* recentProjects READ recentProjects CONSTANT FINAL)
    Q_PROPERTY(ProjectController::ProjectType projectType READ projectType WRITE setProjectType NOTIFY projectTypeChanged FINAL)
    // clang-format on
public:
    // The two kinds of projects the application can handle.
    enum ProjectType
    {
        MusicClip= 0,
        Tutorial
    };
    Q_ENUM(ProjectType)

    explicit ProjectController(QObject* parent= nullptr);

    // Canonical, translatable names of the two project types (index == ProjectType).
    static QStringList projectTypeNames();

    // String representation stored inside the project file.
    static QString projectTypeToString(ProjectType type);
    static ProjectType projectTypeFromString(const QString& s);

    RecentFilesModel* recentVideos() const;
    RecentFilesModel* recentProjects() const;

    ProjectController::ProjectType projectType() const;
    void setProjectType(ProjectController::ProjectType newProjectType);

    QString videoFilePath() const;
    void setVideoFilePath(const QString& newVideoFilePath);

    QString projectFilePath() const;
    void setProjectFilePath(const QString& newProjectFilePath);

public slots:
    void addRecentVideo(const QString& path);
    void addRecentProject(const QString& path);
    void clearRecentVideos();
    void clearRecentProjects();
    void openVideo(const QString& path);
    void openProject(const QString& path);
    void saveProject(const QString& path);

    void loadSettings();
    void writeSettings() const;

signals:
    void videoFilePathChanged();
    void projectFilePathChanged();
    void projectTypeChanged();

private:
    // Derive the default project path from a video path by replacing the
    // file extension with ".json".
    static QString deriveProjectPath(const QString& videoPath);

    QString m_videoFilePath;
    QString m_projectFilePath;
    std::unique_ptr<RecentFilesModel> m_recentVideos;
    std::unique_ptr<RecentFilesModel> m_recentProjects;
    ProjectController::ProjectType m_projectType{ProjectController::MusicClip};
};

#endif // PROJECTCONTROLLER_H
