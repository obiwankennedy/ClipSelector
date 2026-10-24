#include <QDirIterator>
#include <QGuiApplication>
#include <QQmlApplicationEngine>

void showResources()
{
    QDirIterator it(":", QDirIterator::Subdirectories);
    while(it.hasNext())
    {
        qDebug() << it.next();
    }
}

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);

    Q_INIT_RESOURCE(qmake_Views);
    Q_INIT_RESOURCE(qmake_Core);

    // showResources();

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.load(":/qt/qml/ClipApp/Main.qml");
    // engine.loadFromModule("ClipApp", "Main");

    return app.exec();
}
///
