#ifndef MAINCONTROLLER_H
#define MAINCONTROLLER_H

#include <QObject>
#include <QQmlEngine>
#include "clipmodel.h"
#include "audiofilemodel.h"
#include "filteredmodel.h"

class MainController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QString fileName READ fileName WRITE setFileName NOTIFY fileNameChanged FINAL)
    Q_PROPERTY(ClipModel* clipModel READ clipModel CONSTANT)
    Q_PROPERTY(quint64 position READ position WRITE setPosition NOTIFY positionChanged FINAL)
    Q_PROPERTY(AudioFileModel* musics READ musics CONSTANT)
    Q_PROPERTY(FilteredModel* wantedMusics READ wantedMusics CONSTANT)
public:
    explicit MainController(QObject *parent = nullptr);

    QString fileName() const;
    void setFileName(const QString &newFileName);

    ClipModel *clipModel() const;
    AudioFileModel* musics() const;
    FilteredModel* wantedMusics() const;

    quint64 position() const;
    void setPosition(quint64 newPosition);



public slots:
    void reset();
    void saveModel(const QString& uri) const;
    void playScenes();
    void setPattern(const QString& pattern);
    void setMusic(int idx, const QString& song);

signals:
    void fileNameChanged();

    void play();
    void movePosition(quint64 p);
    void positionChanged();


private slots:
    void computePosition();

private:
    QString m_fileName;
    std::unique_ptr<ClipModel> m_clipModel;
    std::unique_ptr<AudioFileModel> m_musics;
    std::unique_ptr<FilteredModel> m_proxyMusic;
    quint64 m_position;
};

#endif // MAINCONTROLLER_H
