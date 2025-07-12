#include "utils.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonObject>

namespace Utils
{
QByteArray model2Json(const QString& file, const ClipModel* model)
{

    auto data= model->allData();
    QJsonObject root;
    root["video"]= file;
    QJsonArray infoArray;

    for(auto const& info : data)
    {
        QJsonObject obj;
        QJsonArray rangesArray;
        auto list= info.ranges->ranges();
        for(auto const& range : list)
        {
            QJsonObject rangeJson;
            rangeJson["begin"]= static_cast<qint64>(range.begin);
            rangeJson["end"]= static_cast<qint64>(range.end);

            rangesArray.append(rangeJson);
        }
        obj["ranges"]= rangesArray;
        obj["label"]= info.label;
        obj["music"]= info.music;
        obj["color"]= info.color.name();
        obj["offset"]= static_cast<qint64>(info.offset);
        infoArray.append(obj);
    }
    root["clips"]= infoArray;
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

    ctrl->setFileName(obj["video"].toString());

    auto clips= obj["clips"].toArray();
    auto model= ctrl->clipModel();

    for(auto const& clipref : clips)
    {
        auto clip= clipref.toObject();
        auto array= clip["ranges"].toArray();
        auto ranges= new RangeModel;
        for(auto obj : array)
        {
            auto json= obj.toObject();
            ranges->addRange(static_cast<quint64>(json["begin"].toInteger()),
                             static_cast<quint64>(json["end"].toInteger()));
        }
        model->insertClip(ClipInfo{clip["label"].toString(), ranges, QColor::fromString(clip["color"].toString()),
                                   clip["music"].toString(), clip["offset"].toInteger()});
    }
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
} // namespace Utils
