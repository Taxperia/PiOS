// SPDX-License-Identifier: GPL-3.0-or-later
#include "FirstBootClient.h"
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
    setPiOSApplicationIdentity(app,QStringLiteral("org.fidan.FirstBoot"));
    QTranslator translator;
    if (QLocale::system().language() == QLocale::Turkish
        && translator.load(QStringLiteral(":/i18n/fidan-firstboot_tr.qm"))) {
        app.installTranslator(&translator);
    }
    QQuickStyle::setStyle(QStringLiteral("Basic"));
    FirstBootClient setup;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("Setup"), &setup);
    engine.loadFromModule(QStringLiteral("Fidan.FirstBoot"), QStringLiteral("FirstBoot"));
    return engine.rootObjects().isEmpty() ? EXIT_FAILURE : app.exec();
}

