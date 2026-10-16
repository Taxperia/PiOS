// SPDX-License-Identifier: GPL-3.0-or-later
#include "GreeterController.h"
#include "../common/IconLookup.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QLocale>
#include <QTranslator>

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    setPiOSApplicationIdentity(app,QStringLiteral("org.fidan.Shell"));
    QTranslator translator;
    if (QLocale::system().language() == QLocale::Turkish
        && translator.load(QStringLiteral(":/i18n/fidan-greeter_tr.qm"))) {
        app.installTranslator(&translator);
    }
    QQuickStyle::setStyle(QStringLiteral("Basic"));
    GreeterController greeter;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("Greeter"), &greeter);
    engine.loadFromModule(QStringLiteral("Fidan.Greeter"), QStringLiteral("Greeter"));
    return engine.rootObjects().isEmpty() ? EXIT_FAILURE : app.exec();
}

