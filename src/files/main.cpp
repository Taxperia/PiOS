#include "../common/IconLookup.h"
#include "../common/AppStyle.h"
// SPDX-License-Identifier: GPL-3.0-or-later
#include "FilesModel.h"
#include "ThemeIconProvider.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include "../common/UiCapture.h"
int main(int argc,char**argv){qputenv("QT_WAYLAND_SHELL_INTEGRATION", "xdg-shell"); QGuiApplication app(argc,argv); setPiOSApplicationIdentity(app,QStringLiteral("org.fidan.Files"));app.setApplicationName("fidan-files");app.setApplicationDisplayName("Fidan Files");QQuickStyle::setStyle("Basic");FilesModel files(argc>1?QString::fromLocal8Bit(argv[1]):QString());AppStyle style; QQmlApplicationEngine engine; engine.rootContext()->setContextProperty("UiStyle", &style);engine.addImageProvider("theme",new ThemeIconProvider);engine.rootContext()->setContextProperty("filesBackend",&files);QObject::connect(&engine,&QQmlApplicationEngine::objectCreationFailed,&app,[]{QCoreApplication::exit(EXIT_FAILURE);},Qt::QueuedConnection);engine.loadFromModule("Fidan.Files","Files");scheduleUiCapture(app); return app.exec();}

