// SPDX-License-Identifier: GPL-3.0-or-later
#include "ProductInfo.h"
#include "ShellController.h"
#include "SystemMonitor.h"
#include "ThemeIconProvider.h"
#include "UpdateController.h"
#include "DisplayController.h"
#include "SettingsData.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include "../common/UiCapture.h"

int main(int argc, char *argv[])
{
    qputenv("QT_WAYLAND_SHELL_INTEGRATION", "xdg-shell"); QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("fidan-settings"));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("Fidan Settings"));
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    SettingsData settingsData(false);
    ProductInfo product;
    SystemMonitor monitor(false);
    ShellController controller(false, false);
    UpdateController updates;
    updates.startAutomaticChecks();
    DisplayController displays;
    QQmlApplicationEngine engine;
    engine.addImageProvider(QStringLiteral("theme"), new ThemeIconProvider);
    engine.rootContext()->setContextProperty(QStringLiteral("SettingsData"), &settingsData);
    engine.rootContext()->setContextProperty(QStringLiteral("Product"), &product);
    engine.rootContext()->setContextProperty(QStringLiteral("SystemMonitor"), &monitor);
    engine.rootContext()->setContextProperty(QStringLiteral("Controller"), &controller);
    engine.rootContext()->setContextProperty(QStringLiteral("Updates"), &updates);
    engine.rootContext()->setContextProperty(QStringLiteral("Displays"), &displays);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, [] { QCoreApplication::exit(EXIT_FAILURE); }, Qt::QueuedConnection);
    const QStringList arguments = app.arguments();
    const int pageArgument = arguments.indexOf(QStringLiteral("--page"));
    engine.rootContext()->setContextProperty(QStringLiteral("InitialSettingsPage"),
        pageArgument >= 0 && pageArgument + 1 < arguments.size() ? arguments.at(pageArgument + 1) : QStringLiteral("System"));
    engine.loadFromModule(QStringLiteral("Fidan.Settings"), QStringLiteral("Settings"));
    scheduleUiCapture(app); return app.exec();
}
