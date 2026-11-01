#include "musiccontroller.h"

MusicController::MusicController(QObject* parent)
    : QObject{parent}, m_musics{new AudioFileModel}, m_proxyMusic{new FilteredModel}
{
    m_proxyMusic->setSourceModel(m_musics.get());
}

AudioFileModel* MusicController::musics() const
{
    return m_musics.get();
}
FilteredModel* MusicController::wantedMusics() const
{
    return m_proxyMusic.get();
}
void MusicController::setPattern(const QString& pattern)
{
    m_proxyMusic->setSearch(pattern);
}