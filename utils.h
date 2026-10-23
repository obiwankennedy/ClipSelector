#ifndef UTILS_H
#define UTILS_H

#include "audiofilemodel.h"
#include "clipmodel.h"
#include "maincontroller.h"
#include <QJsonDocument>

namespace Utils
{

QByteArray model2Json(const QString& name, const ClipModel* model);
void readJson(const QString& file, MainController* ctrl);
void readAudioList(const QString& filename, AudioFileModel* model);
quint64 closiestPositionInClip(const QList<std::pair<quint64, quint64> > &ranges, quint64 pos, bool* ok);
}; // namespace Utils

#endif // UTILS_H
