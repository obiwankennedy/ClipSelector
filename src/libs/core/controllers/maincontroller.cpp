#include "maincontroller.h"

#include <QFileInfo>
#include <QSaveFile>
#include <QSettings>
#include <QTimer>

#include "effectplayback.h"
#include "utils.h"

namespace
{
constexpr int AutoSaveDelayMs = 1500;
} // namespace

MainController::MainController(QObject* parent)
    : QObject{parent}
    , m_videoCtrl{new VideoController}
    , m_musicCtrl{new MusicController}
    , m_renderCtrl{new RenderController}
    , m_tutoCtrl{new TutorialController}
    , m_projectCtrl{new ProjectController}
    , m_effectPlayback{new EffectPlayback(m_tutoCtrl->effects())}
    , m_autoSaveTimer{new QTimer(this)}
{
    connect(m_projectCtrl.get(), &ProjectController::projectFilePathChanged, this,
            &MainController::projectFilePathChanged);
    connect(m_projectCtrl.get(), &ProjectController::projectFilePathChanged, this,
            [this]() { m_tutoCtrl->setProjectFilePath(m_projectCtrl->projectFilePath()); });
    connect(m_projectCtrl.get(), &ProjectController::videoFilePathChanged, this, &MainController::reset);
    connect(m_projectCtrl.get(), &ProjectController::videoFilePathChanged, this,
            [this]() { m_tutoCtrl->setVideoFilePath(m_projectCtrl->videoFilePath()); });

    // Auto-save: any change on the clip model or the effect model (row
    // insertions/removals, resets and data changes, including the property
    // changes forwarded by VideoEffectModel::connectEvent) schedules a write
    // of the project file next to the video, debounced.
    m_autoSaveTimer->setSingleShot(true);
    m_autoSaveTimer->setInterval(AutoSaveDelayMs);
    connect(m_autoSaveTimer.get(), &QTimer::timeout, this,
            [this]() {
                if(!m_projectCtrl->projectFilePath().isEmpty())
                    saveModel(m_projectCtrl->projectFilePath());
            });

    auto* clips= m_videoCtrl->clipModel();
    auto* effects= m_tutoCtrl->effects();
    const auto scheduleSave= [this] { m_autoSaveTimer->start(); };
    connect(clips, &QAbstractListModel::dataChanged, this, scheduleSave);
    connect(clips, &QAbstractListModel::rowsInserted, this, scheduleSave);
    connect(clips, &QAbstractListModel::rowsRemoved, this, scheduleSave);
    connect(clips, &QAbstractListModel::modelReset, this, scheduleSave);
    connect(effects, &QAbstractListModel::dataChanged, this, scheduleSave);
    connect(effects, &QAbstractListModel::rowsInserted, this, scheduleSave);
    connect(effects, &QAbstractListModel::rowsRemoved, this, scheduleSave);
    connect(effects, &QAbstractListModel::modelReset, this, scheduleSave);
    connect(effects, &VideoEffectModel::effectsChanged, this, scheduleSave);

    Utils::readAudioList("/home/renaud/documents/playlist_official.apl", m_musicCtrl->musics());

    loadSettings();
}

void MainController::reset()
{
    m_videoCtrl->reset();
    // A project file was derived for this video (see
    // ProjectController::openVideo): reload it if it already exists.
    const QString projectPath= m_projectCtrl->projectFilePath();
    if(!projectPath.isEmpty() && QFileInfo::exists(projectPath))
        openJson(projectPath);
}

void MainController::openJson(const QString& uri)
{
    setProjectFilePath(QUrl::fromUserInput(uri).toLocalFile());
    Utils::readJson(QUrl::fromUserInput(uri).toLocalFile(), this);
}

void MainController::openProject(const QString& uri)
{
    const QString path= QUrl::fromUserInput(uri).toLocalFile();
    m_projectCtrl->addRecentProject(path);
    openJson(path);
}

void MainController::saveModel(const QString& uri) const
{
    auto const& data= Utils::model2Json(m_projectCtrl->videoFilePath(), m_videoCtrl->clipModel(),
                                        ProjectController::projectTypeToString(m_projectCtrl->projectType()),
                                        m_tutoCtrl->effects());

    auto filePath= QUrl::fromUserInput(uri).toLocalFile();

    if(!filePath.endsWith(".json"))
        filePath+= ".json";

    QSaveFile file(filePath);

    if(file.open(QIODevice::WriteOnly))
    {
        qDebug() << data << uri;
        file.write(data);
        file.commit();
    }
    else
    {
        qDebug() << file.errorString();
    }
}

void MainController::loadSettings()
{
    m_renderCtrl->loadSettings();
    m_projectCtrl->loadSettings();
}
void MainController::writeSettings()
{
    m_renderCtrl->writeSettings();
    m_projectCtrl->writeSettings();
    m_tutoCtrl->writeSettings();
}

QString MainController::projectFilePath() const
{
    return m_projectCtrl->projectFilePath();
}

void MainController::setProjectFilePath(const QString& newProjectFilePath)
{
    m_projectCtrl->setProjectFilePath(newProjectFilePath);
}

VideoController* MainController::videoCtrl() const
{
    return m_videoCtrl.get();
}

MusicController* MainController::musicCtrl() const
{
    return m_musicCtrl.get();
}

RenderController* MainController::renderCtrl() const
{
    return m_renderCtrl.get();
}

TutorialController* MainController::tutoCtrl() const
{
    return m_tutoCtrl.get();
}

ProjectController* MainController::projectCtrl() const
{
    return m_projectCtrl.get();
}

EffectPlayback* MainController::effectPlayback() const
{
    return m_effectPlayback.get();
}
