// SPDX-License-Identifier: GPL-3.0-or-later
#include "PhotoModel.h"
#include "../common/AppStyle.h"
#include "../common/ThemeIconProvider.h"
#include "../common/UiCapture.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QFileInfo>
int main(int argc, char **argv) {
    qputenv("QT_WAYLAND_SHELL_INTEGRATION", "xdg-shell");
    QGuiApplication app(argc, argv);
    app.setApplicationName("fidan-photos"); app.setApplicationDisplayName("Fidan Photos");
    QQuickStyle::setStyle("Basic");
    PhotoModel photos; AppStyle style;
    if (argc > 1) {
        const QString argument = QString::fromLocal8Bit(argv[1]);
        photos.open(argument.startsWith("file:") ? QUrl(argument) : QUrl::fromLocalFile(QFileInfo(argument).absoluteFilePath()));
    }
    QQmlApplicationEngine engine;
    engine.addImageProvider("theme", new ThemeIconProvider);
    engine.rootContext()->setContextProperty("PhotoLibrary", &photos);
    engine.rootContext()->setContextProperty("UiStyle", &style);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app, [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.loadFromModule("Fidan.Photos", "Photos");
    scheduleUiCapture(app); return app.exec();
}
