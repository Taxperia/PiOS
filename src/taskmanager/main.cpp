#include "../common/AppStyle.h"
// SPDX-License-Identifier: GPL-3.0-or-later
#include "ProcessModel.h"
#include "../common/ThemeIconProvider.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include "../common/UiCapture.h"

int main(int argc, char *argv[])
{
    qputenv("QT_WAYLAND_SHELL_INTEGRATION", "xdg-shell"); QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("fidan-task-manager"));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("PiOS Görev Yöneticisi"));
    QQuickStyle::setStyle(QStringLiteral("Basic"));
    ProcessModel processes;
    AppStyle style; QQmlApplicationEngine engine; engine.rootContext()->setContextProperty("UiStyle", &style);
    engine.addImageProvider(QStringLiteral("theme"), new ThemeIconProvider);
    engine.rootContext()->setContextProperty(QStringLiteral("processBackend"), &processes);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, [] { QCoreApplication::exit(EXIT_FAILURE); }, Qt::QueuedConnection);
    engine.rootContext()->setContextProperty(QStringLiteral("InitialTaskPage"),
        app.arguments().contains(QStringLiteral("--performance")) ? QStringLiteral("performance") : QStringLiteral("processes"));
    engine.loadFromModule(QStringLiteral("Fidan.TaskManager"), QStringLiteral("TaskManager"));
    scheduleUiCapture(app); return app.exec();
}
