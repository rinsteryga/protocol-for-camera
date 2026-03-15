#include "QmlImageItem.hpp"
#include "VideoReceiver.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    qmlRegisterType<QmlImageItem>("App", 1, 0, "QmlImage");

    VideoReceiver receiver;
    QQmlApplicationEngine engine;

    engine.rootContext()->setContextProperty("receiver", &receiver);

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

    engine.loadFromModule("VideoReceiver", "Main");

    return app.exec();
}
