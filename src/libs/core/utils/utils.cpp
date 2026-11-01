#include "utils.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonObject>

namespace Utils
{
QByteArray model2Json(const QString& file, const ClipModel* model, const QString& projectType,
                      const VideoEffectModel* effects)
{
    auto const& data= model->allData();
    QJsonObject root;
    root["video"]= file;
    root["projectType"]= projectType;
    QJsonArray infoArray;

    for(auto const& info : data)
    {
        QJsonObject obj;
        QJsonArray rangesArray;
        auto list= info->ranges()->allRanges();
        for(auto const& range : list)
        {
            QJsonObject rangeJson;
            rangeJson["begin"]= static_cast<qint64>(range.first);
            rangeJson["end"]= static_cast<qint64>(range.second);

            rangesArray.append(rangeJson);
        }
        obj["ranges"]= rangesArray;
        obj["label"]= info->label();
        obj["music"]= info->music();
        obj["color"]= info->color().name();
        obj["name"]= info->name();
        obj["offset"]= static_cast<qint64>(info->offset());
        infoArray.append(obj);
    }
    root["clips"]= infoArray;

    if(effects)
    {
        QJsonArray effectsArray;
        for(auto const& e : effects->allEffects())
        {
            QJsonObject obj;
            obj["type"]= static_cast<int>(e->type());
            obj["begin"]= e->begin();
            obj["end"]= e->end();
            e->saveData(obj);
            effectsArray.append(obj);
        }
        root["effects"]= effectsArray;
    }

    QJsonDocument doc;
    doc.setObject(root);
    return doc.toJson();
}

void readJson(const QString& file, MainController* ctrl)
{
    QFile filejson(file);

    if(!filejson.open(QIODevice::ReadOnly))
        qDebug() << "error";

    auto byte= filejson.readAll();

    QJsonDocument doc= QJsonDocument::fromJson(byte);
    auto obj= doc.object();

    auto videoCtrl= ctrl->videoCtrl();
    auto projectCtrl= ctrl->projectCtrl();

    projectCtrl->setVideoFilePath(obj["video"].toString());

    // Restore the project type stored in the file (default: music clip).
    ctrl->projectCtrl()->setProjectType(ProjectController::projectTypeFromString(obj["projectType"].toString()));

    auto clips= obj["clips"].toArray();
    auto model= videoCtrl->clipModel();

    QList<ClipInfo*> infos;
    for(auto const& clipref : std::as_const(clips))
    {
        auto clip= clipref.toObject();
        auto array= clip["ranges"].toArray();
        auto clipInfo= new ClipInfo();

        for(auto obj : std::as_const(array))
        {
            auto json= obj.toObject();
            clipInfo->addRange(static_cast<quint64>(json["begin"].toInteger()),
                               static_cast<quint64>(json["end"].toInteger()));
        }
        clipInfo->setName(clip["name"].toString());
        clipInfo->setColor(QColor::fromString(clip["color"].toString()));
        clipInfo->setMusic(clip["music"].toString());
        clipInfo->setLabel(clip["label"].toString());
        clipInfo->setOffset(clip["offset"].toInteger());

        infos.append(clipInfo);
    }
    model->reset(infos);

    // Restore the timeline effects (tutorial mode).
    auto effectsModel= ctrl->tutoCtrl()->effects();
    QList<VideoTimedEvent*> effectInfos;
    for(auto const& ref : obj["effects"].toArray())
    {
        auto json= ref.toObject();
        auto type= static_cast<VideoTimedEvent::EventType>(json["type"].toInt());
        auto start= static_cast<qint64>(json["begin"].toDouble());
        auto end= static_cast<qint64>(json["end"].toDouble());
        auto* node= VideoEffectModel::createEvent(type);
        if(!node)
            continue;
        node->setBegin(start);
        node->setEnd(end);
        node->readData(json);
        effectInfos.append(node);
    }
    effectsModel->reset(effectInfos);
}

void readAudioList(const QString& filename, AudioFileModel* model)
{
    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly))
        qDebug() << "error text" << filename;

    QJsonDocument doc= QJsonDocument::fromJson(file.readAll());

    auto mainObj= doc.object();

    auto array= mainObj["songs"].toArray();
    QList<QVariantMap> vec;
    for(auto const& value : std::as_const(array))
    {
        auto obj= value.toObject();
        vec.append(QVariantMap({{"path", obj["path"].toString()},
                                {"title", obj["title"].toString()},
                                {"artist", obj["artist"].toString()},
                                {"album", obj["album"].toString()},
                                {"time", obj["time"].toInt()}}));
    }

    auto images= model->dataImage();
    auto imgarray= mainObj["images"].toArray();
    for(auto const& imageObjRef : std::as_const(imgarray))
    {
        auto obj= imageObjRef.toObject();

        auto key= obj["key"].toString();
        auto data= QImage::fromData(QByteArray::fromBase64(obj["img"].toString().toUtf8()));
        images->insert(key, data);
    }

    model->appendSongs(vec);
}

quint64 closiestPositionInClip(const QList<std::pair<quint64, quint64>>& ranges, quint64 pos, bool* ok)
{
    if(ok)
        *ok= true;
    for(auto r : ranges)
    {
        if(pos <= r.first)
        {
            pos= r.first;
            return pos;
        }

        if(pos > r.first && pos < r.second)
            return pos;

        if(pos > r.first)
            continue;
    }

    if(ok)
        *ok= false;
    return pos;
}
} // namespace Utils
