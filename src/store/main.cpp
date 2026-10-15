#include "../common/AppStyle.h"
// SPDX-License-Identifier: GPL-3.0-or-later
#include "StoreModel.h"
#include "ThemeIconProvider.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include "../common/UiCapture.h"

int main(int argc, char *argv[])
{
    qputenv("QT_WAYLAND_SHELL_INTEGRATION", "xdg-shell"); QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("fidan-store"));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("Fidan Store"));
    QQuickStyle::setStyle(QStringLiteral("Basic"));
    StoreModel store;
    AppStyle style; QQmlApplicationEngine engine; engine.rootContext()->setContextProperty("UiStyle", &style);
    engine.addImageProvider(QStringLiteral("theme"), new ThemeIconProvider);
    engine.rootContext()->setContextProperty(QStringLiteral("storeBackend"), &store);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, [] { QCoreApplication::exit(EXIT_FAILURE); }, Qt::QueuedConnection);
    engine.loadFromModule(QStringLiteral("Fidan.Store"), QStringLiteral("Store"));
    scheduleUiCapture(app); return app.exec();
}
