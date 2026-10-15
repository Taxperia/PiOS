// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QGuiApplication>
#include <QQuickWindow>
#include <QTimer>

// Optional deterministic render capture for runtime layout checks.
inline void scheduleUiCapture(QGuiApplication &app)
{
    const QString output = qEnvironmentVariable("FIDAN_UI_CAPTURE");
    if (output.isEmpty()) return;
    QTimer::singleShot(1400, &app, [&app, output] {
        for (QWindow *window : QGuiApplication::topLevelWindows()) {
            auto *quickWindow = qobject_cast<QQuickWindow *>(window);
            if (quickWindow && quickWindow->isVisible()) {
                app.exit(quickWindow->grabWindow().save(output) ? 0 : 1);
                return;
            }
        }
        app.exit(1);
    });
}
