#ifndef MAINCONTROLLER_H
#define MAINCONTROLLER_H

#include "audiofilemodel.h"
#include "clipmodel.h"
#include "filteredmodel.h"
#include <QObject>
#include <QProcess>
#include <QQmlEngine>
#include <QUrl>

class MainController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    // clang-format off
    Q_PROPERTY(QString videoFilePath READ videoFilePath WRITE setVideoFilePath NOTIFY videoFilePathChanged FINAL)
    Q_PROPERTY(QString projectFilePath READ projectFilePath WRITE setProjectFilePath NOTIFY projectFilePathChanged FINAL)
    Q_PROPERTY(ClipModel* clipModel READ clipModel CONSTANT)
    Q_PROPERTY(ClipListModel* clipListModel READ clipListModel CONSTANT)
    Q_PROPERTY(quint64 position READ position WRITE setPosition NOTIFY positionChanged FINAL)
    Q_PROPERTY(AudioFileModel* musics READ musics CONSTANT)
    Q_PROPERTY(FilteredModel* wantedMusics READ wantedMusics CONSTANT)
    Q_PROPERTY(QUrl clipMakeScript READ clipMakeScript WRITE setClipMakeScript NOTIFY clipMakeScriptChanged FINAL)
    Q_PROPERTY(bool busy READ busy WRITE setBusy NOTIFY busyChanged FINAL)
    Q_PROPERTY(MainController::AppMode appMode READ appMode WRITE setAppMode NOTIFY appModeChanged FINAL)
    // clang-format on
public:
    // The two operating modes of the application.
    enum AppMode
    {
        MusicClip= 0,
        Tutorial
    };
    Q_ENUM(AppMode)

    explicit MainController(QObject* parent= nullptr);

    // Canonical, translatable names of the two modes (index == AppMode).
    static QStringList modeNames();

    QString videoFilePath() const;
    void setVideoFilePath(const QString& newFileName);

    ClipModel* clipModel() const;
    ClipListModel* clipListModel() const;
    AudioFileModel* musics() const;
    FilteredModel* wantedMusics() const;

    quint64 position() const;
    void setPosition(quint64 newPosition);

    QUrl clipMakeScript() const;
    void setClipMakeScript(const QUrl& newClipMakeScript);

    void loadSettings();

    bool busy() const;
    void setBusy(bool newBusy);

    MainController::AppMode appMode() const;
    void setAppMode(MainController::AppMode newAppMode);

    QString projectFilePath() const;
    void setProjectFilePath(const QString& newProjectFilePath);

public slots:
    void reset();
    void openJson(const QString& uri);
    void saveModel(const QString& uri) const;
    void playScenes();
    void setPattern(const QString& pattern);
    void setMusic(int idx, const QString& song);
    void setMusicOffset(int idx, int offset);
    int closiestPosition(int idx, qreal pos);
    void writeSettings();
    void runClipMaker(bool keepSize, bool label, bool merge, bool noMusic);

signals:
    void videoFilePathChanged();
    void play();
    void movePosition(quint64 p);
    void positionChanged();
    void clipMakeScriptChanged();
    void busyChanged();
    void appModeChanged();
    void projectFilePathChanged();

private slots:
    void computePosition();

private:
    QString m_videoFilePath;
    std::unique_ptr<ClipModel> m_clipModel;
    std::unique_ptr<ClipListModel> m_clipListModel;
    std::unique_ptr<AudioFileModel> m_musics;
    std::unique_ptr<FilteredModel> m_proxyMusic;
    std::unique_ptr<QProcess> m_runner;
    quint64 m_position;
    QUrl m_clipMakeScript;
    bool m_busy;
    QString m_projectFilePath;
    MainController::AppMode m_appMode{MainController::MusicClip};
};

#endif // MAINCONTROLLER_H
