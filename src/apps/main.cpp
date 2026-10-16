// SPDX-License-Identifier: GPL-3.0-or-later
#include "UtilityModels.h"
#include "../common/IconLookup.h"
#include "../common/AppStyle.h"
#include "../common/ThemeIconProvider.h"
#include "../common/UiCapture.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QFileInfo>
#include <QProcess>
int main(int argc,char **argv) {
    qputenv("QT_WAYLAND_SHELL_INTEGRATION","xdg-shell");
    QGuiApplication app(argc,argv);
    const auto arguments=app.arguments(); QString kind="calculator";
    for(const auto &candidate:{"calculator","clock","calendar","pdf"}) if(arguments.contains("--"+QString(candidate))) kind=candidate;
    const QMap<QString,QString> names={{"calculator","Hesap Makinesi"},{"clock","Saat"},{"calendar","Takvim"},{"pdf","PDF Görüntüleyici"}};
    app.setApplicationName("fidan-"+kind); app.setApplicationDisplayName("PiOS "+names.value(kind));
    setPiOSApplicationIdentity(app,"org.fidan."+QString(kind=="pdf"?"Pdf":kind=="clock"?"Clock":kind=="calendar"?"Calendar":"Calculator"));
    QQuickStyle::setStyle("Basic"); AppStyle style; CalculatorModel calculator; CalendarModel calendar; ClockModel clock; DocumentFiles files;
    if(kind=="pdf") for(int i=1;i<arguments.size();++i) if(!arguments[i].startsWith("--")) { const auto path=arguments[i]; files.open(path.startsWith("file:")?QUrl(path):QUrl::fromLocalFile(QFileInfo(path).absoluteFilePath())); break; }
    QQmlApplicationEngine engine; engine.addImageProvider("theme",new ThemeIconProvider);
    auto *context=engine.rootContext(); context->setContextProperty("UiStyle",&style); context->setContextProperty("AppKind",kind); context->setContextProperty("AppTitle",names.value(kind)); context->setContextProperty("Calc",&calculator); context->setContextProperty("CalendarData",&calendar); context->setContextProperty("ClockData",&clock); context->setContextProperty("Documents",&files);
    QObject::connect(&clock,&ClockModel::timerFinished,&app,[]{ QProcess::startDetached("notify-send",{"--app-name=PiOS Saat","Sayaç tamamlandı","Belirlediğin süre doldu."}); });
    QObject::connect(&engine,&QQmlApplicationEngine::objectCreationFailed,&app,[]{QCoreApplication::exit(1);},Qt::QueuedConnection);
    engine.loadFromModule("Fidan.Apps","Main"); scheduleUiCapture(app); return app.exec();
}
