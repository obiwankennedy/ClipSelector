#include "utils.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QFile>

namespace Utils
{
QByteArray model2Json(const QString& file, const ClipModel* model)
{

    auto data = model->allData();
    QJsonObject root;
    root["video"]=file;
    QJsonArray infoArray;

    for(auto const& info : data)
    {
        QJsonObject obj;
        obj["begin"]=static_cast<qint64>(info.begin);
        obj["end"]=static_cast<qint64>(info.end);
        obj["label"]=info.label;
        obj["music"]=info.music;
        obj["offset"]=static_cast<qint64>(info.offset);
        infoArray.append(obj);
    }
    root["clips"]=infoArray;
    QJsonDocument doc;
    doc.setObject(root);
    return doc.toJson();
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
}
