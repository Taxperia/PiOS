// SPDX-License-Identifier: GPL-3.0-or-later
#include "FirstBootService.h"

#include <QCoreApplication>
#include <QDBusConnection>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    FirstBootService service;
    QDBusConnection bus = QDBusConnection::systemBus();
    if (!bus.registerService(QStringLiteral("org.fidan.FirstBoot1"))
        || !bus.registerObject(QStringLiteral("/org/fidan/FirstBoot1"), &service,
                               QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals)) {
        return EXIT_FAILURE;
    }
    return app.exec();
}

