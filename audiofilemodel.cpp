/***************************************************************************
 *	Copyright (C) 2020 by Renaud Guezennec                               *
 *   http://www.rolisteam.org/contact                                      *
 *                                                                         *
 *   This software is free software; you can redistribute it and/or modify *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "audiofilemodel.h"

#include <QImage>
#include <taglib/attachedpictureframe.h>
#include <taglib/fileref.h>
#include <taglib/id3v1tag.h>
#include <taglib/id3v2frame.h>
#include <taglib/id3v2tag.h>
#include <taglib/mp4tag.h>
#include <taglib/mpegfile.h>
#include <taglib/tlist.h>
#include <taglib/tpropertymap.h>
#include <taglib/tstring.h>
#include <taglib/vorbisfile.h>
#include <taglib/xiphcomment.h>

#include <QtConcurrent/QtConcurrent>

QString timeToText(quint64 second)
{
    int min = second / 60;
    int s = second - (min * 60);
    return QString("%1:%2").arg(min, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
}

QImage readCoverImageFromMP3(const QString& file)
{
    TagLib::MPEG::File mpgTagger(file.toLocal8Bit());
    auto tagv2 = mpgTagger.ID3v2Tag(true);
    if (!tagv2)
        return {};

    TagLib::ID3v2::FrameList frames = tagv2->frameList("APIC");

    if (frames.isEmpty()) {
        // qDebug() << "No picture to read" << file;
        return {};
    }
    TagLib::ID3v2::AttachedPictureFrame* picFrame = static_cast<TagLib::ID3v2::AttachedPictureFrame*>(frames.front());

    // qDebug() << picFrame->mimeType().toCString();
    QImage image;
    image.loadFromData((const uchar*)picFrame->picture().data(), picFrame->picture().size());
    return image;
}

void readMetaData(const std::vector<AudioFileInfo*>& vec, AudioFileModel* model, QHash<QString, QImage>& dataImage)
{
    int i = 0;
    qDebug() << "Beginning meta data reading ";
    std::for_each(vec.begin(), vec.end(), [&i, &model, &dataImage](AudioFileInfo* info) {
        if (info->m_time != 0 && !info->m_title.isEmpty() && !info->m_artist.isEmpty() && !info->m_album.isEmpty()
            && dataImage.contains(QString("%1-%2").arg(info->m_artist, info->m_album))
            && !dataImage[QString("%1-%2").arg(info->m_artist, info->m_album)].isNull())
            return;

        TagLib::FileRef ref(info->m_filepath.toLocal8Bit());
        auto tag = ref.tag();
        auto audio = ref.audioProperties();
        if (tag == nullptr)
            return;
        info->m_title = TStringToQString(tag->title());
        info->m_artist = TStringToQString(tag->artist());
        info->m_album = TStringToQString(tag->album());
        if (audio == nullptr)
            return;
        info->m_time = static_cast<quint64>(ref.audioProperties()->length());

        if (!dataImage.contains(QString("%1-%2").arg(info->m_artist, info->m_album))
            || dataImage[QString("%1-%2").arg(info->m_artist, info->m_album)].isNull()) { // get images
            if (info->m_filepath.endsWith("mp3")) {
                QImage image = readCoverImageFromMP3(info->m_filepath);
                if (!image.isNull()) {
                    dataImage.insert(QString("%1-%2").arg(info->m_artist, info->m_album), image);
                    qDebug() << "Picture has been found." << info->m_title << info->m_artist;
                }
            } else if (info->m_filepath.endsWith("m4a")) {
            }
        }
        model->infoUpdated(i);
        ++i;
    });
    qDebug() << "End meta data reading ";
}

AudioFileModel::AudioFileModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int AudioFileModel::rowCount(const QModelIndex& parent) const
{
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid())
        return 0;

    return static_cast<int>(m_data.size());
}

QVariant AudioFileModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    QVariant var;
    auto& item = m_data.at(static_cast<std::size_t>(index.row()));
    switch (role) {
    case PathRole:
        var = item->m_filepath;
        break;
    case ArtistRole:
        var = item->m_artist;
        break;
    case TitleRole:
        var = item->m_title;
        if (item->m_title.isEmpty())
            var = item->m_filepath;
        break;
    case TimeRole:
        var = timeToText(item->m_time);
        break;
    case IndexRole:
        var = index.row();
        break;
    case ExportSelectedRole: {
        auto it = std::find(std::begin(m_selectedToExport), std::end(m_selectedToExport), index.row());
        var = (it != std::end(m_selectedToExport));
        break;
    } break;
    }

    return var;
}

bool AudioFileModel::appendSongs(const QList<QVariantMap>& pathlist)
{
    beginInsertRows(QModelIndex(), m_data.size(), pathlist.size() + m_data.size());
    std::for_each(pathlist.begin(), pathlist.end(), [this](const QVariantMap& map) {
        std::unique_ptr<AudioFileInfo> info(
            new AudioFileInfo({ map["path"].toString(), map["artist"].toString().trimmed(),
                map["title"].toString().trimmed(), map["album"].toString(), map["time"].toUInt() }));
        m_data.push_back(std::move(info));
    });
    endInsertRows();

    /* QtConcurrent::run([this]() {
         auto songList= songs();
         readMetaData(songList, this, m_dataImage);
     });*/
    return true;
}

AudioFileInfo* AudioFileModel::songInfoAt(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_data.size()))
        return nullptr;

    return m_data.at(static_cast<std::size_t>(index)).get();
}

bool AudioFileModel::removeSongs(const std::vector<int>& songs)
{
    std::for_each(std::begin(songs), std::end(songs), [this](int idx) {
        beginRemoveRows(QModelIndex(), idx, idx);
        m_data.erase(m_data.begin() + idx);
        endRemoveRows();
    });
    return true;
}

QHash<int, QByteArray> AudioFileModel::roleNames() const
{
    static QHash<int, QByteArray> hash({ { PathRole, "path" },
        { ArtistRole, "artist" },
        { TitleRole, "title" },
        { TimeRole, "time" },
        { AlbumRole, "album" },
        { IndexRole, "songIndex" },
        { ExportSelectedRole, "isSelectedForExport" } });
    return hash;
}

std::vector<AudioFileInfo*> AudioFileModel::songs() const
{
    std::vector<AudioFileInfo*> songs;
    songs.reserve(m_data.size());
    std::transform(m_data.begin(), m_data.end(), std::back_inserter(songs),
        [](const std::unique_ptr<AudioFileInfo>& audioFile) { return audioFile.get(); });
    return songs;
}

void AudioFileModel::infoUpdated(int i)
{
    emit dataChanged(index(i, 0), index(i, 0), QVector<int>() << ArtistRole << TitleRole << TimeRole);
}

void AudioFileModel::insertSongAt(int i, const std::vector<QString>& vec)
{

    beginInsertRows(QModelIndex(), i, i + vec.size() - 1);

    std::vector<AudioFileInfo*> songs;
    std::transform(vec.begin(), vec.end(), std::back_inserter(songs), [](const QString& path) {
        auto info = new AudioFileInfo;
        info->m_filepath = path;
        return info;
    });

    int j = 0;
    for (auto info : songs) {
        std::unique_ptr<AudioFileInfo> song(info);
        m_data.insert(m_data.begin() + i + j, std::move(song));
    }
    endInsertRows();

    QtConcurrent::run([this, songs]() { readMetaData(songs, this, m_dataImage); });
}

void AudioFileModel::resetModel()
{
    beginResetModel();
    m_data.clear();
    endRemoveRows();
}
void AudioFileModel::addToExport(int i)
{
    m_selectedToExport.push_back(i);
    emit dataChanged(index(i), index(i), QVector<int> { ExportSelectedRole });
}

void AudioFileModel::cleanExportList()
{
    beginResetModel();
    m_selectedToExport.clear();
    endResetModel();
}

const std::vector<int>& AudioFileModel::exportList() const
{
    return m_selectedToExport;
}

QHash<QString, QImage>* AudioFileModel::dataImage()
{
    return &m_dataImage;
}
