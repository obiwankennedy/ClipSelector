#ifndef MAINCONTROLLER_H
#define MAINCONTROLLER_H

#include "musiccontroller.h"
#include "projectcontroller.h"
#include "rendercontroller.h"
#include "tutorialcontroller.h"
#include "videocontroller.h"

#include "effectplayback.h"

#include <QObject>
#include <QQmlEngine>
#include <QUrl>

class QTimer;

class MainController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    // clang-format off
    Q_PROPERTY(QString projectFilePath READ projectFilePath WRITE setProjectFilePath NOTIFY projectFilePathChanged FINAL)
    Q_PROPERTY(VideoController* videoCtrl READ videoCtrl CONSTANT FINAL)
    Q_PROPERTY(MusicController* musicCtrl READ musicCtrl CONSTANT FINAL)
    Q_PROPERTY(RenderController* renderCtrl READ renderCtrl CONSTANT FINAL)
    Q_PROPERTY(TutorialController* tutoCtrl READ tutoCtrl CONSTANT FINAL)
    Q_PROPERTY(ProjectController* projectCtrl READ projectCtrl CONSTANT FINAL)
    Q_PROPERTY(EffectPlayback* effectPlayback READ effectPlayback CONSTANT FINAL)
    // clang-format on
public:
    explicit MainController(QObject* parent= nullptr);

    void loadSettings();

    QString projectFilePath() const;
    void setProjectFilePath(const QString& newProjectFilePath);

    VideoController* videoCtrl() const;
    MusicController* musicCtrl() const;
    RenderController* renderCtrl() const;

    TutorialController* tutoCtrl() const;

    ProjectController* projectCtrl() const;

    // Makes the live playback follow the timeline effects (cut / speed /
    // comment audio).
    EffectPlayback* effectPlayback() const;

public slots:
    void reset();
    void openJson(const QString& uri);
    void openProject(const QString& uri);
    void saveModel(const QString& uri) const;
    void writeSettings();

signals:
    void projectFilePathChanged();

private:
    std::unique_ptr<VideoController> m_videoCtrl;
    std::unique_ptr<RenderController> m_renderCtrl;
    std::unique_ptr<MusicController> m_musicCtrl;
    std::unique_ptr<TutorialController> m_tutoCtrl;
    std::unique_ptr<ProjectController> m_projectCtrl;
    std::unique_ptr<EffectPlayback> m_effectPlayback;

    // Debounces model changes and auto-saves the project file next to the
    // video. (Re)started on every clip / effect mutation.
    std::unique_ptr<QTimer> m_autoSaveTimer;
};

#endif // MAINCONTROLLER_H
