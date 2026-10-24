#include "tutorialcomment.h"

#include <QUuid>

#include <algorithm>

TutorialComment::TutorialComment(QObject* parent)
    : QObject(parent)
    , m_id(QUuid::createUuid().toString(QUuid::WithoutBraces).left(12))
{
}

QString TutorialComment::id() const
{
    return m_id;
}

QString TutorialComment::text() const
{
    return m_text;
}

void TutorialComment::setText(const QString& newText)
{
    if(m_text == newText)
        return;
    m_text= newText;
    // Editing the text invalidates any previously generated audio.
    if(m_status == Ready)
        m_status= Outdated;
    m_error.clear();
    emit textChanged();
    emit statusChanged();
}

qint64 TutorialComment::startTimeMs() const
{
    return m_startTimeMs;
}

void TutorialComment::setStartTimeMs(qint64 newStartTimeMs)
{
    if(m_startTimeMs == newStartTimeMs)
        return;
    m_startTimeMs= newStartTimeMs;
    // Moving a comment on the timeline does NOT invalidate its audio.
    emit startTimeMsChanged();
}

QString TutorialComment::audioFile() const
{
    return m_audioFile;
}

void TutorialComment::setAudioFile(const QString& newAudioFile)
{
    if(m_audioFile == newAudioFile)
        return;
    m_audioFile= newAudioFile;
    emit audioFileChanged();
}

qint64 TutorialComment::audioDurationMs() const
{
    return m_audioDurationMs;
}

void TutorialComment::setAudioDurationMs(qint64 newAudioDurationMs)
{
    if(m_audioDurationMs == newAudioDurationMs)
        return;
    m_audioDurationMs= newAudioDurationMs;
    emit audioDurationMsChanged();
}

QString TutorialComment::language() const
{
    return m_language;
}

void TutorialComment::setLanguage(const QString& newLanguage)
{
    if(m_language == newLanguage)
        return;
    m_language= newLanguage;
    if(m_status == Ready)
        m_status= Outdated;
    m_error.clear();
    emit languageChanged();
    emit statusChanged();
}

QString TutorialComment::voice() const
{
    return m_voice;
}

void TutorialComment::setVoice(const QString& newVoice)
{
    if(m_voice == newVoice)
        return;
    m_voice= newVoice;
    if(m_status == Ready)
        m_status= Outdated;
    m_error.clear();
    emit voiceChanged();
    emit statusChanged();
}

int TutorialComment::status() const
{
    return m_status;
}

void TutorialComment::setStatus(int newStatus)
{
    if(m_status == newStatus)
        return;
    m_status= newStatus;
    emit statusChanged();
}

QString TutorialComment::error() const
{
    return m_error;
}

void TutorialComment::setError(const QString& newError)
{
    if(m_error == newError)
        return;
    m_error= newError;
    emit errorChanged();
}

bool TutorialComment::isStale() const
{
    // Stale when the current (text, language, voice) differ from the snapshot
    // taken at the last successful generation. The status is intentionally NOT
    // part of this check: editing text/language/voice flips the status to
    // Outdated, and we still want isStale() to report that regeneration is
    // required for the changed source.
    return m_generatedText != m_text
        || m_generatedLanguage != m_language
        || m_generatedVoice != m_voice;
}

void TutorialComment::markGenerated()
{
    m_generatedText= m_text;
    m_generatedLanguage= m_language;
    m_generatedVoice= m_voice;
    m_status= Ready;
    m_error.clear();
    emit statusChanged();
}

TutorialCommentModel::TutorialCommentModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

TutorialCommentModel::~TutorialCommentModel()
{
    qDeleteAll(m_data);
}

int TutorialCommentModel::rowCount(const QModelIndex& parent) const
{
    if(parent.isValid())
        return 0;
    return m_data.size();
}

QVariant TutorialCommentModel::data(const QModelIndex& index, int role) const
{
    if(!index.isValid() || index.row() < 0 || index.row() >= m_data.size())
        return {};

    auto c= m_data.at(index.row());
    switch(role)
    {
    case Id:
        return c->id();
    case Text:
        return c->text();
    case StartTimeMs:
        return c->startTimeMs();
    case AudioFile:
        return c->audioFile();
    case AudioDurationMs:
        return c->audioDurationMs();
    case Language:
        return c->language();
    case Voice:
        return c->voice();
    case Status:
        return c->status();
    case Error:
        return c->error();
    case CommentRole:
        return QVariant::fromValue(c);
    default:
        return {};
    }
}

bool TutorialCommentModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if(!index.isValid() || index.row() < 0 || index.row() >= m_data.size())
        return false;

    auto c= m_data.at(index.row());
    switch(role)
    {
    case Text:
        c->setText(value.toString());
        break;
    case StartTimeMs:
        c->setStartTimeMs(value.toLongLong());
        break;
    case Language:
        c->setLanguage(value.toString());
        break;
    case Voice:
        c->setVoice(value.toString());
        break;
    case Error:
        c->setError(value.toString());
        break;
    default:
        return false;
    }
    emit dataChanged(index, index);
    return true;
}

QHash<int, QByteArray> TutorialCommentModel::roleNames() const
{
    return {{Id, "id"},
            {Text, "text"},
            {StartTimeMs, "startTimeMs"},
            {AudioFile, "audioFile"},
            {AudioDurationMs, "audioDurationMs"},
            {Language, "language"},
            {Voice, "voice"},
            {Status, "status"},
            {Error, "error"},
            {CommentRole, "comment"}};
}

Qt::ItemFlags TutorialCommentModel::flags(const QModelIndex& index) const
{
    if(!index.isValid())
        return Qt::NoItemFlags;
    return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

TutorialComment* TutorialCommentModel::commentAt(int row) const
{
    if(row < 0 || row >= m_data.size())
        return nullptr;
    return m_data.at(row);
}

int TutorialCommentModel::indexOfId(const QString& id) const
{
    for(int i= 0; i < m_data.size(); ++i)
        if(m_data.at(i)->id() == id)
            return i;
    return -1;
}

QString TutorialCommentModel::addComment(const QString& text, qint64 startTimeMs,
                                         const QString& language, const QString& voice)
{
    auto c= new TutorialComment;
    c->setText(text);
    c->setStartTimeMs(startTimeMs);
    c->setLanguage(language);
    c->setVoice(voice);

    beginInsertRows(QModelIndex(), m_data.size(), m_data.size());
    m_data.append(c);
    endInsertRows();
    emit commentsChanged();
    return c->id();
}

void TutorialCommentModel::removeComment(int row)
{
    if(row < 0 || row >= m_data.size())
        return;
    beginRemoveRows(QModelIndex(), row, row);
    delete m_data.takeAt(row);
    endRemoveRows();
    emit commentsChanged();
}

void TutorialCommentModel::clear()
{
    beginResetModel();
    qDeleteAll(m_data);
    m_data.clear();
    endResetModel();
    emit commentsChanged();
}

void TutorialCommentModel::reset(const QList<TutorialComment*>& infos)
{
    beginResetModel();
    m_data= infos;
    endResetModel();
    emit commentsChanged();
}

QList<TutorialComment*> TutorialCommentModel::sortedByTime() const
{
    auto sorted= m_data;
    // Stable sort by start time (insertion order preserved for ties).
    std::stable_sort(std::begin(sorted), std::end(sorted),
                     [](const TutorialComment* a, const TutorialComment* b) {
                         return a->startTimeMs() < b->startTimeMs();
                     });
    return sorted;
}

QList<TutorialComment*> TutorialCommentModel::allComments() const
{
    return m_data;
}
