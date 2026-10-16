#include "../common/IconLookup.h"
// SPDX-License-Identifier: GPL-3.0-or-later
#include "AppModel.h"
#include "DesktopModel.h"
#include "../files/FilesModel.h"
#include "LayerShellBridge.h"
#include "NotificationService.h"
#include "ProductInfo.h"
#include "ShellController.h"
#include "SystemMonitor.h"
#include "ThemeIconProvider.h"
#include "UpdateController.h"
#include "DisplayController.h"
#include "SettingsData.h"

#include <QCommandLineParser>
#include <QDBusConnection>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QLocale>
#include <QTimer>
#include <QTranslator>
#include <QStandardPaths>

#ifdef FIDAN_HAS_LAYER_SHELL
#include <LayerShellQt/Shell>
#endif

int main(int argc, char *argv[])
{
    QGuiApplication::setApplicationName(QStringLiteral("fidan-shell"));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("Project Fidan Desktop"));
    QGuiApplication::setOrganizationName(QStringLiteral("Project Fidan"));
    QGuiApplication::setOrganizationDomain(QStringLiteral("fidan.org"));

    QGuiApplication app(argc, argv); setPiOSApplicationIdentity(app,QStringLiteral("org.fidan.Shell"));
    QTranslator translator;
    if (QLocale::system().language() == QLocale::Turkish
        && translator.load(QStringLiteral(":/i18n/fidan-shell_tr.qm"))) {
        app.installTranslator(&translator);
    }
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Project Fidan Wayland desktop shell"));
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption previewOption(QStringList{QStringLiteral("p"), QStringLiteral("preview")},
                                     QStringLiteral("Run the desktop inside one development window."));
    QCommandLineOption mockOption(QStringList{QStringLiteral("m"), QStringLiteral("mock")},
                                  QStringLiteral("Use deterministic mock system data."));
    QCommandLineOption screenshotOption(QStringList{QStringLiteral("s"), QStringLiteral("screenshot")},
                                        QStringLiteral("Save the preview window to an image and exit."),
                                        QStringLiteral("path"));
    QCommandLineOption contextMenuOption(QStringLiteral("show-context-menu"),
                                         QStringLiteral("Open the desktop context menu in preview mode."));
    parser.addOption(previewOption);
    parser.addOption(mockOption);
    parser.addOption(screenshotOption);
    parser.addOption(contextMenuOption);
    parser.process(app);

    const bool previewMode = parser.isSet(previewOption)
        || qEnvironmentVariableIsSet("FIDAN_PREVIEW_MODE")
        || QGuiApplication::platformName() != QStringLiteral("wayland");
    const bool mockMode = parser.isSet(mockOption) || qEnvironmentVariableIsSet("FIDAN_MOCK_SYSTEM");

#ifdef FIDAN_HAS_LAYER_SHELL
    if (!previewMode) {
        LayerShellQt::Shell::useLayerShell();
    }
#endif

    QQuickStyle::setStyle(QStringLiteral("Basic"));

    UpdateController updates;
    if (!mockMode)
        updates.startAutomaticChecks();
    DisplayController displays(mockMode);
    SettingsData settingsData(mockMode);
    ProductInfo product;
    SystemMonitor systemMonitor(mockMode);
    ShellController controller(previewMode, mockMode);
    LayerShellBridge layerShell(!previewMode);
    AppModel applications;
    DesktopModel desktopItems;
    FilesModel desktopFiles(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
    NotificationService notifications;
    QObject::connect(&desktopFiles, &FilesModel::busyChanged, &notifications, [&] {
        if (desktopFiles.busy()) return;
        desktopItems.refresh();
        notifications.Notify(QStringLiteral("Files"), 0, {},
            desktopFiles.errorText().isEmpty() ? QStringLiteral("Desktop files updated") : QStringLiteral("File operation failed"),
            desktopFiles.errorText(), {}, {}, 5000);
    });
    QObject::connect(&controller, &ShellController::actionFailed, &notifications,
        [&notifications](const QString &message) {
            notifications.Notify(QStringLiteral("Project Fidan"), 0, QStringLiteral("dialog-warning"),
                QObject::tr("Action could not be completed"), message, {}, {}, 7000);
        });

    QDBusConnection::sessionBus().registerService(QStringLiteral("org.fidan.Shell"));
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/Shell"), &controller,
                                                  QDBusConnection::ExportAllSlots);
    notifications.registerOnSessionBus();

    QQmlApplicationEngine engine;
    engine.addImageProvider(QStringLiteral("theme"), new ThemeIconProvider);
    engine.rootContext()->setContextProperty(QStringLiteral("SettingsData"), &settingsData);
    engine.rootContext()->setContextProperty(QStringLiteral("Updates"), &updates);
    engine.rootContext()->setContextProperty(QStringLiteral("Displays"), &displays);
    engine.rootContext()->setContextProperty(QStringLiteral("Product"), &product);
    engine.rootContext()->setContextProperty(QStringLiteral("SystemMonitor"), &systemMonitor);
    engine.rootContext()->setContextProperty(QStringLiteral("Controller"), &controller);
    engine.rootContext()->setContextProperty(QStringLiteral("LayerShell"), &layerShell);
    engine.rootContext()->setContextProperty(QStringLiteral("Applications"), &applications);
    engine.rootContext()->setContextProperty(QStringLiteral("DesktopItems"), &desktopItems);
    engine.rootContext()->setContextProperty(QStringLiteral("DesktopFiles"), &desktopFiles);
    engine.rootContext()->setContextProperty(QStringLiteral("Notifications"), &notifications);
    engine.rootContext()->setContextProperty(QStringLiteral("PreviewContextMenu"),
                                             parser.isSet(contextMenuOption));

    engine.rootContext()->setContextProperty(QStringLiteral("NestedSession"), qEnvironmentVariableIsSet("FIDAN_NESTED_SESSION"));
    engine.rootContext()->setContextProperty(QStringLiteral("PreviewCapture"), parser.isSet(screenshotOption));

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, [] { QCoreApplication::exit(EXIT_FAILURE); }, Qt::QueuedConnection);
    engine.loadFromModule(QStringLiteral("Fidan.Shell"), QStringLiteral("Main"));
    // The integration is initialized for this process; URL handlers and any
    // other descendants must inherit the normal application surface role.
    qputenv("QT_WAYLAND_SHELL_INTEGRATION", "xdg-shell");

    if (parser.isSet(screenshotOption)) {
        const QString screenshotPath = parser.value(screenshotOption);
        QTimer::singleShot(1500, &app, [&app, screenshotPath] {
            for (QWindow *window : QGuiApplication::topLevelWindows()) {
                auto *quickWindow = qobject_cast<QQuickWindow *>(window);
                if (quickWindow && quickWindow->isVisible()) {
                    const bool saved = quickWindow->grabWindow().save(screenshotPath);
                    app.exit(saved ? EXIT_SUCCESS : EXIT_FAILURE);
                    return;
                }
            }
            app.exit(EXIT_FAILURE);
        });
    }

    return app.exec();
}

