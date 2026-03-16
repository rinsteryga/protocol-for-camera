#include <QCoreApplication>
#include "VideoSender.hpp"

int main(int argc, char *argv[])
{
    // Используем QCoreApplication для работы без GUI
    QCoreApplication app(argc, argv);

    VideoSender sender;
    // Можно добавить вывод в консоль, что сервер запущен
    qInfo() << "Video Server is running. Press Ctrl+C to stop.";

    return app.exec();
}
