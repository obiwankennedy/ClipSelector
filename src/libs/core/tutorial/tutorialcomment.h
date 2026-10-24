#ifndef TUTORIALCOMMENT_H
#define TUTORIALCOMMENT_H

#include <QAbstractListModel>
#include <QColor>
#include <QObject>
#include <QQmlEngine>
#include <QString>

// A single narrated comment in TUTORIAL mode.
//
// Timeline convention: every time is expressed in MILLISECONDS on the source
// video timeline (the same unit QtMultimedia's MediaPlayer::position uses).
class TutorialComment : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged FINAL)
    Q_PROPERTY(qint64 startTimeMs READ startTimeMs WRITE setStartTimeMs NOTIFY startTimeMsChanged FINAL)
    Q_PROPERTY(QString audioFile READ audioFile WRITE setAudioFile NOTIFY audioFileChanged FINAL)
    Q_PROPERTY(qint64 audioDurationMs READ audioDurationMs WRITE setAudioDurationMs NOTIFY audioDurationMsChanged FINAL)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged FINAL)
    Q_PROPERTY(QString voice READ voice WRITE setVoice NOTIFY voiceChanged FINAL)
    Q_PROPERTY(int status READ status NOTIFY statusChanged FINAL)
    Q_PROPERTY(QString error READ error WRITE setError NOTIFY errorChanged FINAL)
public:
    enum GenerationStatus
    {
        Pending = 0, // never generated
        Generating,  // generation in flight
        Ready,       // audio generated and up to date
        Outdated,    // text/language/voice changed since generation
        Error        // last generation failed
    };
    Q_ENUM(GenerationStatus)

    explicit TutorialComment(QObject* parent= nullptr);

    QString id() const;

    QString text() const;
    void setText(const QString& newText);

    qint64 startTimeMs() const;
    void setStartTimeMs(qint64 newStartTimeMs);

    QString audioFile() const;
    void setAudioFile(const QString& newAudioFile);

    qint64 audioDurationMs() const;
    void setAudioDurationMs(qint64 newAudioDurationMs);

    QString language() const;
    void setLanguage(const QString& newLanguage);

    QString voice() const;
    void setVoice(const QString& newVoice);

    int status() const;
    void setStatus(int newStatus);

    QString error() const;
    void setError(const QString& newError);

    // True when generated audio exists but the text/language/voice it was
    // produced from no longer matches the current values. A position change
    // (setStartTimeMs) deliberately does NOT make the audio stale.
    bool isStale() const;

    // Records that audio was just successfully generated with the current
    // text/language/voice and marks the comment Ready.
    void markGenerated();

signals:
    void textChanged();
    void startTimeMsChanged();
    void audioFileChanged();
    void audioDurationMsChanged();
    void languageChanged();
    void voiceChanged();
    void statusChanged();
    void errorChanged();

private:
    QString m_id;
    QString m_text;
    qint64 m_startTimeMs{0};
    QString m_audioFile;
    qint64 m_audioDurationMs{0};
    QString m_language;
    QString m_voice;
    int m_status{Pending};
    QString m_error;

    // Snapshot of the values the generated audio was produced from.
    QString m_generatedText;
    QString m_generatedLanguage;
    QString m_generatedVoice;
};

// List model of TutorialComment.
//
// The model keeps the user's insertion order (so the editor table is stable
// while editing); subtitle/rendering consumers use sortedByTime() to obtain a
// deterministic timeline ordering.
class TutorialCommentModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
public:
    enum DataRole
    {
        Id= Qt::UserRole + 1,
        Text,
        StartTimeMs,
        AudioFile,
        AudioDurationMs,
        Language,
        Voice,
        Status,
        Error,
        CommentRole
    };
    Q_ENUM(DataRole)

    TutorialCommentModel(QObject* parent= nullptr);
    ~TutorialCommentModel() override;

    int rowCount(const QModelIndex& parent= QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    QHash<int, QByteArray> roleNames() const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    TutorialComment* commentAt(int row) const;

    // Returns the row of the comment with the given id, or -1.
    int indexOfId(const QString& id) const;

    // Insert a new comment (created with the given values) and return its id.
    QString addComment(const QString& text, qint64 startTimeMs,
                       const QString& language, const QString& voice);

    void removeComment(int row);
    void clear();

    // Replace the whole model with the given comments (takes ownership).
    void reset(const QList<TutorialComment*>& infos);

    // All comments in timeline order (stable for equal start times).
    QList<TutorialComment*> sortedByTime() const;

    // All comments in insertion order.
    QList<TutorialComment*> allComments() const;
signals:
    void commentsChanged();

private:
    QList<TutorialComment*> m_data;
};

#endif // TUTORIALCOMMENT_H
