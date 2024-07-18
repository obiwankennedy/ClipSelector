#ifndef UTILS_H
#define UTILS_H

#include "clipmodel.h"
#include "audiofilemodel.h"
#include <QJsonDocument>


namespace Utils
{

QByteArray model2Json(const QString& name, const ClipModel* model);
void readAudioList(const QString& filename, AudioFileModel* model);
};

#endif // UTILS_H
