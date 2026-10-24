#ifndef AUDIOSPECT_H
#define AUDIOSPECT_H

#include <QQuickItem>
#include <QSGNode>

#include <QAudioDecoder>

class AudioSampleNode : public QSGNode
{
public:
    AudioSampleNode();

    void update(const QRectF &rect, int durationTotal, const std::vector<std::vector<double> >& samples);
};

class AudioSpect : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString audioFile READ audioFile WRITE setAudioFile NOTIFY audioFileChanged FINAL)
public:
    AudioSpect();

    QString audioFile() const;
    void setAudioFile(const QString &newAudioFile);

protected:
    QSGNode * updatePaintNode(QSGNode *oldNode, QQuickItem::UpdatePaintNodeData *updatePaintNodeData) override;
signals:
    void audioFileChanged();
private:
    QString m_audioFile;
    QAudioDecoder m_deco;
    bool m_audioChanged{false};
};

#endif // AUDIOSPECT_H
