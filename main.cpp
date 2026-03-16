#include "VideoSender.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    VideoSender sender;
    QQmlApplicationEngine engine;

    engine.rootContext()->setContextProperty("sender", &sender);

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("VideoSender", "Main");

    return app.exec();
}
