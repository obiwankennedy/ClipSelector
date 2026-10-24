#include "subtitlegenerator.h"

#include <QDir>
#include <QFileInfo>
#include <QSaveFile>

#include <algorithm>

QString SubtitleGenerator::formatTimestamp(qint64 ms)
{
    if(ms < 0)
        ms= 0;
    const qint64 totalMs= ms;
    const int mms= static_cast<int>(totalMs % 1000);
    const qint64 totalSec= totalMs / 1000;
    const int sec= static_cast<int>(totalSec % 60);
    const int min= static_cast<int>((totalSec / 60) % 60);
    const int hr= static_cast<int>(totalSec / 3600);
    return QString("%1:%2:%3,%4")
        .arg(hr, 2, 10, QChar('0'))
        .arg(min, 2, 10, QChar('0'))
        .arg(sec, 2, 10, QChar('0'))
        .arg(mms, 3, 10, QChar('0'));
}

qint64 SubtitleGenerator::computeEndMs(qint64 startMs, qint64 durationMs)
{
    qint64 dur= durationMs;
    if(dur <= 0)
        dur= kFallbackDurationMs;
    if(dur < kMinDurationMs)
        dur= kMinDurationMs;
    return startMs + dur;
}

QByteArray SubtitleGenerator::generateSrt(const QList<Entry>& entries,
                                          QStringList* problems)
{
    auto sorted= entries;
    std::stable_sort(std::begin(sorted), std::end(sorted),
                     [](const Entry& a, const Entry& b) {
                         return a.startMs < b.startMs;
                     });

    QString doc;
    int seq= 1;
    for(const auto& e : sorted)
    {
        const qint64 endMs= computeEndMs(e.startMs, e.endMs - e.startMs);
        if(e.endMs - e.startMs <= 0)
        {
            if(problems)
                problems->append("Comment at " + formatTimestamp(e.startMs)
                                 + " had no positive duration; using fallback of "
                                 + QString::number(kFallbackDurationMs) + "ms.");
        }

        doc += QString::number(seq++);
        doc += "\n";
        doc += formatTimestamp(e.startMs) + " --> " + formatTimestamp(endMs);
        doc += "\n";
        doc += e.text;
        doc += "\n\n";
    }

    return doc.toUtf8();
}

bool SubtitleGenerator::writeSrt(const QString& filePath, const QByteArray& srt,
                                 QString* error)
{
    // Create only the parent directory of the target file. Using
    // QFileInfo(...).absolutePath() strips the file name; QDir(filePath) would
    // treat the file name as a directory component and mkpath would create
    // "subtitles.srt" as a directory, making the subsequent open() fail.
    QDir().mkpath(QFileInfo(filePath).absolutePath());
    QSaveFile file(filePath);
    if(!file.open(QIODevice::WriteOnly))
    {
        if(error)
            *error= "Could not open subtitle file for writing: " + filePath;
        return false;
    }
    file.write(srt);
    if(!file.commit())
    {
        if(error)
            *error= file.errorString();
        return false;
    }
    return true;
}