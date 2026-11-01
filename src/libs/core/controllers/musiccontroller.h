#ifndef MUSICCONTROLLER_H
#define MUSICCONTROLLER_H

#include "audiofilemodel.h"
#include "filteredmodel.h"
#include <QObject>
#include <QQmlEngine>

class MusicController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(AudioFileModel* musics READ musics CONSTANT)
    Q_PROPERTY(FilteredModel* wantedMusics READ wantedMusics CONSTANT)
public:
    explicit MusicController(QObject* parent= nullptr);

    AudioFileModel* musics() const;
    FilteredModel* wantedMusics() const;

public slots:
    void setPattern(const QString& pattern);
signals:

private:
    std::unique_ptr<AudioFileModel> m_musics;
    std::unique_ptr<FilteredModel> m_proxyMusic;
};

#endif // MUSICCONTROLLER_H
