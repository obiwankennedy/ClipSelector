#ifndef SUBTITLEGENERATOR_H
#define SUBTITLEGENERATOR_H

#include <QByteArray>
#include <QList>
#include <QString>

// Pure SRT subtitle generation for TUTORIAL mode.
//
// The generator is deliberately decoupled from the data model so it can be
// unit-tested without any media files: it takes a flat list of (text, start,
// end) entries and produces a valid UTF-8 SRT document.
class SubtitleGenerator
{
public:
    struct Entry
    {
        QString text;
        qint64 startMs{0};
        qint64 endMs{0};
    };

    // Minimum visible duration (ms) for a subtitle cue.
    static constexpr qint64 kMinDurationMs= 1000;
    // Fallback duration (ms) applied when a comment has no known audio length.
    static constexpr qint64 kFallbackDurationMs= 3000;

    // Format a millisecond value as an SRT timestamp: "HH:MM:SS,mmm".
    static QString formatTimestamp(qint64 ms);

    // Compute a cue end time from a start time and an (optional) duration.
    // Guarantees end > start by applying kMinDurationMs, and falls back to
    // kFallbackDurationMs when the duration is non-positive (unknown).
    static qint64 computeEndMs(qint64 startMs, qint64 durationMs);

    // Build the SRT document from the given entries.
    //
    // - entries are sorted by start time (stable);
    // - every entry appears exactly once with a valid sequence number;
    // - overlapping cues are preserved (each keeps its own timing);
    // - zero/negative durations are replaced by the fallback and reported via
    //   `problems` (never silently produced).
    //
    // Returns the SRT content as UTF-8 bytes.
    static QByteArray generateSrt(const QList<Entry>& entries,
                                  QStringList* problems= nullptr);

    // Write the SRT document to `filePath` atomically (temp file + rename).
    // Returns true on success.
    static bool writeSrt(const QString& filePath, const QByteArray& srt,
                         QString* error= nullptr);
};

#endif // SUBTITLEGENERATOR_H
