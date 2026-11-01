#ifndef RENDERCONTROLLER_H
#define RENDERCONTROLLER_H

#include <QObject>
#include <QProcess>
#include <QQmlEngine>

class RenderController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool busy READ busy WRITE setBusy NOTIFY busyChanged FINAL)
    Q_PROPERTY(QUrl clipMakeScript READ clipMakeScript WRITE setClipMakeScript NOTIFY clipMakeScriptChanged FINAL)

public:
    explicit RenderController(QObject* parent= nullptr);

    bool busy() const;
    void setBusy(bool newBusy);
    QUrl clipMakeScript() const;
    void setClipMakeScript(const QUrl& newClipMakeScript);

    void loadSettings();

public slots:
    void runClipMaker(bool keepSize, bool label, bool merge, bool noMusic, const QString& dest);
    void writeSettings();

signals:
    void busyChanged();
    void clipMakeScriptChanged();
    void projectFilePathChanged();

private:
    std::unique_ptr<QProcess> m_runner;
    bool m_busy{false};
    QUrl m_clipMakeScript;
};

#endif // RENDERCONTROLLER_H
