#include "rendercontroller.h"

#include <QSettings>

RenderController::RenderController(QObject* parent) : QObject{parent}
{
    m_runner.reset(new QProcess());
    QProcessEnvironment env= QProcessEnvironment::systemEnvironment();
    env.insert("PATH", QString("%1:%2").arg("/home/renaud/www/scripts/venv/bin", env.value("PATH")));
    env.insert("VIRTUAL_ENV_PROMPT", QString("(venv)"));
    env.insert("VIRTUAL_ENV", QString("/home/renaud/www/scripts/venv"));
    m_runner->setProcessEnvironment(env);

    connect(m_runner.get(), &QProcess::started, this, [this]() { setBusy(true); });
    connect(m_runner.get(), &QProcess::finished, this, [this]() { setBusy(false); });

    connect(m_runner.get(), &QProcess::errorOccurred, this, [this]() { qDebug() << "ERROR while running ClipMake"; });
    connect(m_runner.get(), &QProcess::readyReadStandardError, this,
            [this]() { qDebug() << m_runner->readAllStandardError(); });
    connect(m_runner.get(), &QProcess::readyReadStandardOutput, this,
            [this]() { qDebug() << m_runner->readAllStandardOutput(); });
}

bool RenderController::busy() const
{
    return m_busy;
}

void RenderController::setBusy(bool newBusy)
{
    if(m_busy == newBusy)
        return;
    m_busy= newBusy;
    emit busyChanged();
}

void RenderController::loadSettings()
{
    QSettings settings("rolisteam", "ClipSelector");
    m_clipMakeScript
        = settings.value("clipmakerscript", "file:///home/renaud/www/scripts/08_videos/clipmaker.py").toUrl();
}
void RenderController::writeSettings()
{
    QSettings settings("rolisteam", "ClipSelector");

    settings.setValue("clipmakerscript", m_clipMakeScript);
}

void RenderController::runClipMaker(bool keepSize, bool label, bool merge, bool noMusic, const QString& dest)
{
    if(m_runner->state() != QProcess::NotRunning)
        return;
    QStringList args;

    if(keepSize)
        args << "-k";
    if(label)
        args << "-l";
    if(merge)
        args << "-m";
    if(noMusic)
        args << "-n";

    args << "-c" << QUrl::fromUserInput(dest).toLocalFile();
    m_runner->start(m_clipMakeScript.toLocalFile(), args);
    // proc.setEnvironment();
}

QUrl RenderController::clipMakeScript() const
{
    return m_clipMakeScript;
}

void RenderController::setClipMakeScript(const QUrl& newClipMakeScript)
{
    if(m_clipMakeScript == newClipMakeScript)
        return;
    m_clipMakeScript= newClipMakeScript;
    emit clipMakeScriptChanged();
}
