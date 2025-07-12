#include "audiospect.h"

#include <QAudioBuffer>
#include <QAudioFormat>
#include <QSGSimpleRectNode>
#include <audiorw.hpp>

AudioSampleNode::AudioSampleNode() {}

void AudioSampleNode::update(const QRectF& rect, int durationTotal, const std::vector<std::vector<double>>& samples)
{

    auto sampleCount= std::max_element(std::begin(samples), std::end(samples),
                                       [](const std::vector<double>& a, const std::vector<double>& b)
                                       { return a.size() < b.size(); })
                          ->size();
    int resolution= sampleCount / rect.width();
    auto const pixPerSample= 1.0;

    int idx= 0;
    for(int i= resolution; i < (int)sampleCount; i+= resolution)
    {
        qreal l= 0.;
        qreal r= 0.;
        int begin= i - resolution;
        for(int j= 0; j < resolution; ++j)
        {
            l+= std::abs(samples[0][begin + j]);
            if(samples.size() == 2)
                r+= std::abs(samples[1][begin + j]);
            else
                r+= std::abs(samples[0][begin + j]);
        }
        l/= resolution;
        r/= resolution;

        auto halfH= (rect.height() / 2);

        auto y= halfH - (l * 2) * halfH;
        auto h= (l * 2) * halfH + (r * 2) * halfH;

        QRectF dest{pixPerSample * idx, y, pixPerSample, h};
        qDebug() << "rect:" << y << ": " << h;
        auto node= new QSGSimpleRectNode(dest, Qt::blue);
        appendChildNode(node);
        idx++;
    }

    markDirty(QSGNode::DirtyGeometry);
}

AudioSpect::AudioSpect()
{
    setFlag(QQuickItem::ItemHasContents, true);

    connect(this, &AudioSpect::audioFileChanged, this, [this]() { update(); });
    connect(&m_deco, QOverload<QAudioDecoder::Error>::of(&QAudioDecoder::error), this,
            [this]() { qDebug() << "error:" << m_deco.errorString(); });
}

QSGNode* AudioSpect::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* updatePaintNodeData)
{
    AudioSampleNode* n= static_cast<AudioSampleNode*>(oldNode);
    if(!n)
    {
        n= new AudioSampleNode();
    }
    qDebug() << "updatePaintNode:";

    // auto buffer = m_deco.read();
    double sample_rate;
    try
    {
        std::vector<std::vector<double>> audio
            = audiorw::read(QUrl(m_audioFile).toLocalFile().toStdString(), sample_rate);
        qDebug() << "sample_rate:" << sample_rate;
        n->update(boundingRect(), m_deco.duration(), audio);
    }
    catch(const std::exception& e)
    {
        qDebug() << e.what();
    }

    return n;
}

QString AudioSpect::audioFile() const
{
    return m_audioFile;
}

void AudioSpect::setAudioFile(const QString& newAudioFile)
{
    if(m_audioFile == newAudioFile)
        return;
    qDebug() << "setAudioFile:";
    m_audioFile= newAudioFile;
    m_audioChanged= true;
    emit audioFileChanged();
}
