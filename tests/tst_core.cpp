// SPDX-License-Identifier: GPL-3.0-or-later
#include "ProductInfo.h"
#include "SystemMonitor.h"
#include "NotificationService.h"
#include "ShellController.h"
#include "AppModel.h"
#include "DesktopModel.h"
#include "StoreModel.h"
#include "ProcessModel.h"
#include "FilesModel.h"
#include "../src/common/DesktopLaunch.h"
#include "../src/firstboot/FirstBootValidation.h"

#include <QFile>
#include <QUuid>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QProcess>
#include <QTemporaryDir>
#include <QElapsedTimer>
#include <QScopeGuard>
#include <QtTest>
#ifdef Q_OS_LINUX
#include <sys/statvfs.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

class CoreTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void loadsProductManifest();
    void fallsBackForInvalidManifest();
    void exposesBoundedMockMetrics();
    void storesAndClearsNotificationHistory();
    void controlsMockSystem();
    void loadsStoreCatalog();
    void installsFlatpakWithoutBlocking_data();
    void installsFlatpakWithoutBlocking();
    void reportsAptAuthorizationFailure();
    void discoversFlatpakAndPreservesSearch();
    void readsAndTerminatesOwnedProcess();
    void createsDesktopApplicationShortcut();
    void performsRealFileOperations();
    void rejectsMissingFolderWithoutChangingLocation();
    void isolatesApplicationWindowEnvironment();
    void synchronizesPreferencesAcrossApplications();
    void readsRealMemoryMetrics();
    void opensDesktopFolderWithSpacesAndDots();
    void opensTextDocumentWithSpacesInEditor();
    void sortsDesktopItems();
    void clearsSearchWhenChangingFolders();
    void keepsHistoryWhenFolderDisappears();
    void measuresStorageWithoutCountingReservedBlocks();
    void createsKnownSidebarLocation();
    void copiesAndMovesTreesWithoutOverwriting();
    void rejectsRecursiveCopyAndRemoteDrop();
    void restoresTrashWithoutOverwriting();
    void notificationActionsAndReplacementTimers();
    void respectsDefaultApplicationChoice();
    void pastesClipboardWithoutKeyboardFocus();
    void rejectsFirstBootCredentialInjection();
    void refusesUntrustedDesktopLaunchers();
};

void CoreTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}


void CoreTest::pastesClipboardWithoutKeyboardFocus()
{
#ifdef Q_OS_LINUX
    QTemporaryDir tools, source, target;
    const QString name = QStringLiteral("Desktop clipboard.txt");
    QFile file(source.filePath(name)); QVERIFY(file.open(QIODevice::WriteOnly)); file.write("desktop paste"); file.close();
    QFile reader(tools.filePath(QStringLiteral("wl-paste"))); QVERIFY(reader.open(QIODevice::WriteOnly));
    reader.write("#!/bin/sh\nprintf 'copy\\n%s\\n' \"$FIDAN_CLIPBOARD_TEST_URL\"\n"); reader.close();
    QVERIFY(reader.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner));
    const QByteArray oldPath = qgetenv("PATH");
    const auto restore = qScopeGuard([oldPath] { qputenv("PATH", oldPath); qunsetenv("FIDAN_CLIPBOARD_TEST_URL"); });
    qputenv("PATH", tools.path().toLocal8Bit() + ':' + oldPath);
    qputenv("FIDAN_CLIPBOARD_TEST_URL", QUrl::fromLocalFile(file.fileName()).toEncoded());
    FilesModel files(target.path());
    files.refreshClipboard();
    QTRY_VERIFY(files.canPaste());
    files.paste();
    QTRY_VERIFY_WITH_TIMEOUT(!files.busy(), 5000);
    QVERIFY2(files.errorText().isEmpty(), qPrintable(files.errorText()));
    QFile result(target.filePath(name)); QVERIFY(result.open(QIODevice::ReadOnly)); QCOMPARE(result.readAll(), QByteArray("desktop paste"));
#endif
}

void CoreTest::copiesAndMovesTreesWithoutOverwriting()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir dir(temp.path());
    QVERIFY(dir.mkpath(QStringLiteral("source/İşler/nested")));
    QVERIFY(dir.mkpath(QStringLiteral("target")));
    const QString source = dir.filePath(QStringLiteral("source/İşler"));
    QFile original(source + QStringLiteral("/nested/.notes.txt"));
    QVERIFY(original.open(QIODevice::WriteOnly)); original.write("preserved contents"); original.close();
#ifdef Q_OS_LINUX
    QVERIFY(::symlink("nested/.notes.txt", QFile::encodeName(source + QStringLiteral("/shortcut")).constData()) == 0);
#else
    QVERIFY(QFile::link(source + QStringLiteral("/nested/.notes.txt"), source + QStringLiteral("/shortcut")));
#endif
    FilesModel files(dir.filePath(QStringLiteral("source")));
    files.copyEntries({0}, false);
    files.setPath(dir.filePath(QStringLiteral("target")));
    QVERIFY(files.canPaste());
    files.paste();
    QTRY_VERIFY_WITH_TIMEOUT(!files.busy(), 5000);
    QVERIFY2(files.errorText().isEmpty(), qPrintable(files.errorText()));
    const QString copied = dir.filePath(QStringLiteral("target/İşler/nested/.notes.txt"));
    QFile content(copied); QVERIFY(content.open(QIODevice::ReadOnly)); QCOMPARE(content.readAll(), QByteArray("preserved contents")); content.close();
    QVERIFY(QFileInfo(dir.filePath(QStringLiteral("target/İşler/shortcut"))).isSymbolicLink());
#ifdef Q_OS_LINUX
    QCOMPARE(QFileInfo(dir.filePath(QStringLiteral("target/İşler/shortcut"))).symLinkTarget(), copied);
#endif
    files.paste();
    QTRY_VERIFY_WITH_TIMEOUT(!files.busy(), 5000);
    QVERIFY(files.errorText().contains(QStringLiteral("Already exists")));
    QVERIFY(QFileInfo::exists(original.fileName()));
    files.setPath(dir.filePath(QStringLiteral("target/İşler/nested")));
    QCOMPARE(files.rowCount(), 0);
    files.setShowHidden(true);
    QCOMPARE(files.rowCount(), 1);
    QCOMPARE(files.properties(0).value(QStringLiteral("name")).toString(), QStringLiteral(".notes.txt"));
    files.copyEntries({0}, true);
    files.setPath(dir.filePath(QStringLiteral("target")));
    files.paste();
    QTRY_VERIFY_WITH_TIMEOUT(!files.busy(), 5000);
    QVERIFY2(files.errorText().isEmpty(), qPrintable(files.errorText()));
    QVERIFY(!QFileInfo::exists(copied));
    QVERIFY(QFileInfo::exists(dir.filePath(QStringLiteral("target/.notes.txt"))));
    QVERIFY(!files.canPaste());
}

void CoreTest::rejectsRecursiveCopyAndRemoteDrop()
{
    QTemporaryDir temp;
    QVERIFY(QDir(temp.path()).mkpath(QStringLiteral("Folder/Child")));
    FilesModel files(temp.path());
    files.dropUrls({QUrl::fromLocalFile(temp.filePath(QStringLiteral("Folder")))}, temp.filePath(QStringLiteral("Folder/Child")), false);
    QTRY_VERIFY_WITH_TIMEOUT(!files.busy(), 5000);
    QVERIFY(files.errorText().contains(QStringLiteral("inside itself")));
    QVERIFY(!QFileInfo::exists(temp.filePath(QStringLiteral("Folder/Child/Folder"))));
    files.dropUrls({QUrl(QStringLiteral("https://example.invalid/file"))}, temp.path(), false);
    QVERIFY(files.errorText().contains(QStringLiteral("local")));
    QVERIFY(!files.busy());
#ifdef Q_OS_LINUX
    const QString pipe = temp.filePath(QStringLiteral("named-pipe"));
    QVERIFY(::mkfifo(QFile::encodeName(pipe).constData(), 0600) == 0);
    files.dropUrls({QUrl::fromLocalFile(pipe)}, temp.filePath(QStringLiteral("Folder/Child")), false);
    QTRY_VERIFY_WITH_TIMEOUT(!files.busy(), 5000);
    QVERIFY(!files.errorText().isEmpty());
    QVERIFY(!QFileInfo::exists(temp.filePath(QStringLiteral("Folder/Child/named-pipe"))));
    QVERIFY(QFileInfo::exists(pipe));
#endif
}

void CoreTest::restoresTrashWithoutOverwriting()
{
    QTemporaryDir temp;
    FilesModel files(temp.path());
    const QString trash = files.placePath(QStringLiteral("trash"));
    const QString name = QStringLiteral("fidan-restore-") + QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QString path = QDir(trash).filePath(name);
    const QString metadata = QDir(trash).filePath(QStringLiteral("../info/") + name + QStringLiteral(".trashinfo"));
    QVERIFY(QDir().mkpath(trash)); QVERIFY(QDir().mkpath(QFileInfo(metadata).absolutePath()));
    const auto cleanup = qScopeGuard([path, metadata] { QFile::remove(path); QFile::remove(metadata); });
    QFile trashed(path); QVERIFY(trashed.open(QIODevice::WriteOnly)); trashed.write("trash content"); trashed.close();
    const QString destination = temp.filePath(QStringLiteral("Original name.txt"));
    QFile original(destination); QVERIFY(original.open(QIODevice::WriteOnly)); original.write("existing"); original.close();
    QFile info(metadata); QVERIFY(info.open(QIODevice::WriteOnly)); info.write("[Trash Info]\nPath=" + QUrl::toPercentEncoding(destination, "/") + "\nDeletionDate=2026-10-15T12:00:00\n"); info.close();
    files.openPlace(QStringLiteral("trash"));
    files.setFilter(name);
    QCOMPARE(files.rowCount(), 1);
    files.restoreEntries({0});
    QTRY_VERIFY_WITH_TIMEOUT(!files.busy(), 5000);
    QVERIFY(!files.errorText().isEmpty());
    QVERIFY(QFileInfo::exists(path)); QVERIFY(QFileInfo::exists(metadata));
    QVERIFY(original.open(QIODevice::ReadOnly)); QCOMPARE(original.readAll(), QByteArray("existing")); original.close();
    QVERIFY(QFile::remove(destination));
    files.restoreEntries({0});
    QTRY_VERIFY_WITH_TIMEOUT(!files.busy(), 5000);
    QVERIFY2(files.errorText().isEmpty(), qPrintable(files.errorText()));
    QVERIFY(original.open(QIODevice::ReadOnly)); QCOMPARE(original.readAll(), QByteArray("trash content"));
    QVERIFY(!QFileInfo::exists(path)); QVERIFY(!QFileInfo::exists(metadata));
}

void CoreTest::notificationActionsAndReplacementTimers()
{
    NotificationService notifications;
    QSignalSpy actions(&notifications, &NotificationService::ActionInvoked);
    QSignalSpy closed(&notifications, &NotificationService::NotificationClosed);
    const uint id = notifications.Notify(QStringLiteral("Test"), 0, {}, QStringLiteral("Old"), {}, {QStringLiteral("open"), QStringLiteral("Open")}, {}, 40);
    QCOMPARE(notifications.actions().size(), 1);
    QCOMPARE(notifications.Notify(QStringLiteral("Test"), id, {}, QStringLiteral("New"), {}, {QStringLiteral("open"), QStringLiteral("Open")}, {}, 0), id);
    QCOMPARE(notifications.history().size(), 1);
    QTest::qWait(70);
    QVERIFY(notifications.visible());
    notifications.invokeAction(id, QStringLiteral("invalid")); QCOMPARE(actions.size(), 0);
    notifications.invokeAction(id, QStringLiteral("open")); QCOMPARE(actions.size(), 1);
    QCOMPARE(actions.first().at(0).toUInt(), id); QCOMPARE(actions.first().at(1).toString(), QStringLiteral("open"));
    QVERIFY(!notifications.visible()); QCOMPARE(closed.first().at(1).toUInt(), 2u);
    notifications.invokeAction(id, QStringLiteral("open")); QCOMPARE(actions.size(), 1);
    QVERIFY(!notifications.history().first().toMap().value(QStringLiteral("active")).toBool());
    notifications.Notify(QStringLiteral("Test"), 0, {}, {}, {}, {}, {}, 10);
    QTRY_COMPARE(closed.size(), 2);
    QCOMPARE(closed.last().at(1).toUInt(), 1u);
}

void CoreTest::respectsDefaultApplicationChoice()
{
#ifdef Q_OS_LINUX
    QTemporaryDir tools;
    const QString id = QStringLiteral("org.fidan.TestDefault.desktop");
    const QString desktop = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation) + QLatin1Char('/') + id;
    QVERIFY(QDir().mkpath(QFileInfo(desktop).absolutePath()));
    QFile entry(desktop); QVERIFY(entry.open(QIODevice::WriteOnly)); entry.write("[Desktop Entry]\nType=Application\nName=Fidan Default Test\nExec=true\nMimeType=application/x-fidan-test;text/plain;\n"); entry.close();
    const auto remove = qScopeGuard([desktop] { QFile::remove(desktop); });
    const QString capture = tools.filePath(QStringLiteral("choice"));
    QFile tool(tools.filePath(QStringLiteral("xdg-mime"))); QVERIFY(tool.open(QIODevice::WriteOnly));
    tool.write("#!/bin/sh\nif [ \"$1\" = default ]; then printf '%s\\n' \"$2\" \"$3\" > \"$FIDAN_DEFAULT_CAPTURE\"; fi\n"); tool.close();
    QVERIFY(tool.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner));
    const QByteArray oldPath = qgetenv("PATH");
    const auto restore = qScopeGuard([oldPath] { qputenv("PATH", oldPath); qunsetenv("FIDAN_DEFAULT_CAPTURE"); });
    qputenv("PATH", tools.path().toLocal8Bit() + ':' + oldPath); qputenv("FIDAN_DEFAULT_CAPTURE", capture.toLocal8Bit());
    ShellController controller(true, true);
    QVERIFY(controller.setDefaultApplication(QStringLiteral("text/plain"), id));
    QFile result(capture); QVERIFY(result.open(QIODevice::ReadOnly)); QCOMPARE(result.readAll(), (id + QStringLiteral("\ntext/plain\n")).toUtf8());
    QVERIFY(!controller.setDefaultApplication(QStringLiteral("text/plain"), QStringLiteral("missing.desktop")));
    QVERIFY(!controller.setDefaultApplication(QStringLiteral("unsupported/type"), id));
#endif
}

void CoreTest::createsKnownSidebarLocation()
{
    QTemporaryDir profile;
    QVERIFY(profile.isValid());
    const QByteArray previousHome = qgetenv("HOME");
    const auto restoreHome = qScopeGuard([previousHome] { qputenv("HOME", previousHome); });
    qputenv("HOME", profile.path().toUtf8());
    FilesModel files(profile.path());
    const QString projects = profile.filePath(QStringLiteral("Projects"));
    QCOMPARE(files.placePath(QStringLiteral("projects")), projects);
    QVERIFY(!QFileInfo::exists(projects));
    QVERIFY(files.openPlace(QStringLiteral("projects")));
    QCOMPARE(files.path(), projects);
    QVERIFY(QFileInfo(projects).isDir());
    QVERIFY(!files.openPlace(QStringLiteral("../unexpected")));
    QCOMPARE(files.path(), projects);
    QVERIFY(!QFileInfo::exists(profile.filePath(QStringLiteral("unexpected"))));
}

void CoreTest::loadsProductManifest()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("product.json"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(R"({
        "id":"org.example.Test",
        "name":"Test Desktop",
        "version":"9.1",
        "channel":"beta",
        "supportUrl":"https://example.invalid/support",
        "accent":"#112233",
        "accentDark":"#001122",
        "wallpaper":"/does/not/exist.png"
    })");
    file.close();

    ProductInfo product(path);
    QCOMPARE(product.id(), QStringLiteral("org.example.Test"));
    QCOMPARE(product.name(), QStringLiteral("Test Desktop"));
    QCOMPARE(product.version(), QStringLiteral("9.1"));
    QCOMPARE(product.channel(), QStringLiteral("beta"));
    QCOMPARE(product.accent(), QStringLiteral("#112233"));
    QVERIFY(product.wallpaperUrl().isLocalFile());
}

void CoreTest::fallsBackForInvalidManifest()
{
    ProductInfo product(QStringLiteral("/missing/product.json"));
    QCOMPARE(product.id(), QStringLiteral("org.fidan.Desktop"));
    QCOMPARE(product.name(), QStringLiteral("Project Fidan"));
    QCOMPARE(product.channel(), QStringLiteral("alpha"));
}

void CoreTest::exposesBoundedMockMetrics()
{
    SystemMonitor monitor(true);
    monitor.refresh();
    QVERIFY(monitor.cpuUsage() >= 0.0 && monitor.cpuUsage() <= 100.0);
    QVERIFY(monitor.memoryUsage() >= 0.0 && monitor.memoryUsage() <= 100.0);
    QVERIFY(monitor.storageUsage() >= 0.0 && monitor.storageUsage() <= 100.0);
    QVERIFY(monitor.temperature() > 0.0);
    QVERIFY(monitor.temperatureAvailable());
    QCOMPARE(monitor.deviceModel(), QStringLiteral("Raspberry Pi 5"));
}

void CoreTest::storesAndClearsNotificationHistory()
{
    NotificationService notifications;
    const uint id = notifications.Notify(QStringLiteral("Test application"), 0, {},
                                         QStringLiteral("Update ready"),
                                         QStringLiteral("A package update is available."),
                                         {}, {}, 0);
    QVERIFY(id > 0);
    QVERIFY(notifications.visible());
    QCOMPARE(notifications.history().size(), 1);
    QCOMPARE(notifications.history().first().toMap().value(QStringLiteral("summary")).toString(),
             QStringLiteral("Update ready"));

    notifications.dismissCurrent();
    QVERIFY(!notifications.visible());
    QCOMPARE(notifications.history().size(), 1);
    notifications.clearHistory();
    QVERIFY(notifications.history().isEmpty());
}

void CoreTest::controlsMockSystem()
{
    ShellController controller(true, true);
    QVERIFY(controller.brightnessAvailable());

    const bool initialWifi = controller.wifiEnabled();
    controller.toggleWifi();
    QCOMPARE(controller.wifiEnabled(), !initialWifi);

    const bool initialBluetooth = controller.bluetoothEnabled();
    controller.toggleBluetooth();
    QCOMPARE(controller.bluetoothEnabled(), !initialBluetooth);

    controller.setBrightness(74);
    QCOMPARE(controller.brightness(), 74);
    controller.setVolume(47);
    QCOMPARE(controller.volume(), 47);

}

void CoreTest::loadsStoreCatalog()
{
    StoreModel store;
    QTRY_VERIFY_WITH_TIMEOUT(!store.busy(), 12000);
    QVERIFY(store.rowCount() >= 5);
    store.setFilter(QStringLiteral("LibreOffice"));
    QCOMPARE(store.rowCount(), 1);
    QCOMPARE(store.data(store.index(0), StoreModel::NameRole).toString(), QStringLiteral("LibreOffice"));

    store.setFilter({});
    store.setCategory(QStringLiteral("Graphics"));
    QVERIFY(store.rowCount() >= 4);
    QCOMPARE(store.data(store.index(0), StoreModel::NameRole).toString(), QStringLiteral("GIMP"));

    store.setCategory({});
    store.setInstalledOnly(true);
    for (int row = 0; row < store.rowCount(); ++row) {
        QVERIFY(store.data(store.index(row), StoreModel::InstalledRole).toBool());
    }
}

void CoreTest::installsFlatpakWithoutBlocking_data()
{
    QTest::addColumn<bool>("remoteFailure");
    QTest::addColumn<bool>("packageFailure");
    QTest::addColumn<bool>("verificationFailure");
    QTest::newRow("successful-install") << false << false << false;
    QTest::newRow("unavailable-source") << true << false << false;
    QTest::newRow("failed-download") << false << true << false;
    QTest::newRow("unverified-install-and-retry") << false << false << true;
}

void CoreTest::installsFlatpakWithoutBlocking()
{
#ifdef Q_OS_LINUX
    QFETCH(bool, remoteFailure);
    QFETCH(bool, packageFailure);
    QFETCH(bool, verificationFailure);
    QTemporaryDir tools;
    QVERIFY(tools.isValid());
    const auto writeTool = [&tools](const QString &name, const QByteArray &body) {
        QFile file(tools.filePath(name));
        return file.open(QIODevice::WriteOnly) && file.write(body) == body.size()
            && file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    };
    QVERIFY(writeTool(QStringLiteral("flatpak"),
        "#!/bin/sh\ncase \"$1\" in\n"
        "info) test -f \"$FIDAN_STORE_TEST_STATE\" && test \"$FIDAN_STORE_TEST_UNVERIFIED\" != 1 ;;\n"
        "remote-add) echo 'Configuring application source'; /bin/sleep 1; if [ \"$FIDAN_STORE_TEST_REMOTE_FAIL\" = 1 ]; then echo 'Source unavailable' >&2; exit 1; fi ;;\n"
        "install) if [ \"$FIDAN_STORE_TEST_PACKAGE_FAIL\" = 1 ]; then echo 'Runtime download failed: connection interrupted' >&2; exit 1; fi; printf '%s\\n' \"$@\" > \"$FIDAN_STORE_TEST_STATE\" ;;\n"
        "*) exit 0 ;;\nesac\n"));
    QVERIFY(writeTool(QStringLiteral("apt"), "#!/bin/sh\nexit 0\n"));
    QVERIFY(writeTool(QStringLiteral("dpkg-query"), "#!/bin/sh\nexit 1\n"));
    const QByteArray oldPath = qgetenv("PATH");
    const auto restore = qScopeGuard([oldPath] {
        qputenv("PATH", oldPath);
        qunsetenv("FIDAN_STORE_TEST_STATE");
        qunsetenv("FIDAN_STORE_TEST_REMOTE_FAIL");
        qunsetenv("FIDAN_STORE_TEST_PACKAGE_FAIL");
        qunsetenv("FIDAN_STORE_TEST_UNVERIFIED");
    });
    qputenv("PATH", tools.path().toLocal8Bit() + ':' + oldPath);
    const QString state = tools.filePath(QStringLiteral("installed"));
    qputenv("FIDAN_STORE_TEST_STATE", state.toLocal8Bit());
    qputenv("FIDAN_STORE_TEST_REMOTE_FAIL", remoteFailure ? "1" : "0");
    qputenv("FIDAN_STORE_TEST_PACKAGE_FAIL", packageFailure ? "1" : "0");
    qputenv("FIDAN_STORE_TEST_UNVERIFIED", verificationFailure ? "1" : "0");
    StoreModel store;
    QTRY_VERIFY_WITH_TIMEOUT(!store.busy(), 12000);
    store.setFilter(QStringLiteral("GIMP"));
    QCOMPARE(store.rowCount(), 1);
    QVERIFY(!store.data(store.index(0), StoreModel::InstalledRole).toBool());
    QElapsedTimer timer;
    timer.start();
    store.installOrRemove(0);
    QVERIFY2(timer.elapsed() < 500, "Remote configuration blocked the UI thread");
    QVERIFY(store.busy());
    QVERIFY(store.data(store.index(0), StoreModel::EntryBusyRole).toBool());
    QTRY_VERIFY_WITH_TIMEOUT(store.operationOutput().contains(QStringLiteral("Configuring application source")), 500);
    QVERIFY(store.busy());
    if (verificationFailure) {
        QTRY_VERIFY_WITH_TIMEOUT(!store.busy(), 5000);
        QVERIFY(store.errorText().contains(QStringLiteral("could not be verified")));
        QVERIFY(!store.data(store.index(0), StoreModel::InstalledRole).toBool());
        QVERIFY(store.canRetry());
        qputenv("FIDAN_STORE_TEST_UNVERIFIED", "0");
        store.retry();
        QTRY_VERIFY_WITH_TIMEOUT(!store.busy(), 5000);
        QVERIFY(store.errorText().isEmpty());
        QVERIFY(store.data(store.index(0), StoreModel::InstalledRole).toBool());
    } else if (remoteFailure || packageFailure) {
        QTRY_VERIFY_WITH_TIMEOUT(!store.errorText().isEmpty(), 5000);
        QVERIFY(store.errorText().contains(remoteFailure ? QStringLiteral("Source unavailable")
                                                        : QStringLiteral("Runtime download failed: connection interrupted")));
        QVERIFY(store.operationOutput().contains(store.errorText()));
        QVERIFY(!QFileInfo::exists(state));
        QVERIFY(!store.data(store.index(0), StoreModel::InstalledRole).toBool());
    } else {
        QTRY_VERIFY_WITH_TIMEOUT(store.data(store.index(0), StoreModel::InstalledRole).toBool(), 5000);
        QVERIFY(store.errorText().isEmpty());
        QFile arguments(state);
        QVERIFY(arguments.open(QIODevice::ReadOnly));
        QCOMPARE(arguments.readAll(), QByteArray("install\n--user\n--assumeyes\nflathub\norg.gimp.GIMP\n"));
    }
    QVERIFY(!store.busy());
    QVERIFY(!store.data(store.index(0), StoreModel::EntryBusyRole).toBool());
#endif
}

void CoreTest::reportsAptAuthorizationFailure()
{
#ifdef Q_OS_LINUX
    QTemporaryDir tools;
    QVERIFY(tools.isValid());
    for (const QString &name : {QStringLiteral("pkexec"), QStringLiteral("apt-get"),
                               QStringLiteral("apt"), QStringLiteral("dpkg-query"), QStringLiteral("flatpak")}) {
        QFile file(tools.filePath(name));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(name == QStringLiteral("pkexec")
            ? "#!/bin/sh\nprintf '%s\\n' \"$@\" > \"$FIDAN_STORE_TEST_STATE\"\necho 'Authorization cancelled' >&2\nexit 126\n"
            : "#!/bin/sh\nexit 1\n");
        QVERIFY(file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner));
    }
    const QByteArray oldPath = qgetenv("PATH");
    const auto restore = qScopeGuard([oldPath] {
        qputenv("PATH", oldPath);
        qunsetenv("FIDAN_STORE_TEST_STATE");
    });
    qputenv("PATH", tools.path().toLocal8Bit() + ':' + oldPath);
    const QString capture = tools.filePath(QStringLiteral("arguments"));
    qputenv("FIDAN_STORE_TEST_STATE", capture.toLocal8Bit());
    StoreModel store;
    QTRY_VERIFY_WITH_TIMEOUT(!store.busy(), 12000);
    store.setFilter(QStringLiteral("QTerminal"));
    QCOMPARE(store.rowCount(), 1);
    store.installOrRemove(0);
    QTRY_VERIFY_WITH_TIMEOUT(!store.errorText().isEmpty(), 5000);
    QVERIFY(store.errorText().contains(QStringLiteral("Authorization cancelled")));
    QVERIFY(!store.busy());
    QVERIFY(!store.data(store.index(0), StoreModel::InstalledRole).toBool());
    QFile arguments(capture);
    QVERIFY(arguments.open(QIODevice::ReadOnly));
    QCOMPARE(arguments.readAll(), (tools.filePath(QStringLiteral("apt-get"))
        + QStringLiteral("\n-oAPT::Status-Fd=1\n-oDpkg::Use-Pty=0\ninstall\n-y\n--no-remove\nqterminal\n")).toUtf8());
#endif
}

void CoreTest::discoversFlatpakAndPreservesSearch()
{
    const QString root = QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation))
                             .filePath(QStringLiteral("flatpak/exports/share/applications"));
    QVERIFY(QDir().mkpath(root));
    const QString path = QDir(root).filePath(QStringLiteral("org.fidan.FlatpakDiscoveryTest.desktop"));
    const auto cleanup = qScopeGuard([path] { QFile::remove(path); });
    QFile desktop(path);
    QVERIFY(desktop.open(QIODevice::WriteOnly | QIODevice::NewOnly));
    desktop.write("[Desktop Entry]\nType=Application\nName=Fidan Flatpak Discovery Test\n"
                  "Icon=org.fidan.FlatpakDiscoveryTest\nExec=flatpak run org.fidan.FlatpakDiscoveryTest\n");
    desktop.close();
    AppModel applications;
    applications.setFilter(QStringLiteral("Fidan Flatpak Discovery Test"));
    QCOMPARE(applications.rowCount(), 1);
    applications.refresh();
    QCOMPARE(applications.rowCount(), 1);
    QCOMPARE(applications.data(applications.index(0), AppModel::DesktopIdRole).toString(),
             QStringLiteral("org.fidan.FlatpakDiscoveryTest.desktop"));
    // Installed apps may have a long display name that omits their familiar
    // command name (for example GNU Image Manipulation Program / GIMP).
    applications.setFilter(QStringLiteral("FlatpakDiscoveryTest"));
    QCOMPARE(applications.rowCount(), 1);
}

void CoreTest::readsAndTerminatesOwnedProcess()
{
#ifdef Q_OS_LINUX
    QProcess sleeper;
    sleeper.start(QStringLiteral("/bin/sleep"), {QStringLiteral("30")});
    QVERIFY(sleeper.waitForStarted());

    ProcessModel processes;
    processes.refresh();
    bool found = false;
    for (int row = 0; row < processes.rowCount(); ++row) {
        if (processes.data(processes.index(row), ProcessModel::PidRole).toInt() == sleeper.processId()) {
            found = true;
            break;
        }
    }
    QVERIFY(found);
    QVERIFY(processes.terminateProcess(static_cast<int>(sleeper.processId())));
    QVERIFY(sleeper.waitForFinished(3000));
#endif
}

void CoreTest::createsDesktopApplicationShortcut()
{
    const QString applicationsPath = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    const QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QVERIFY(QDir().mkpath(applicationsPath));
    QVERIFY(QDir().mkpath(desktopPath));
    const QString desktopId = QStringLiteral("org.fidan.ShortcutTest.desktop");
    const QString sourcePath = QDir(applicationsPath).filePath(desktopId);
    const QString targetPath = QDir(desktopPath).filePath(desktopId);
    QFile::remove(sourcePath);
    QFile::remove(targetPath);

    QFile source(sourcePath);
    QVERIFY(source.open(QIODevice::WriteOnly | QIODevice::NewOnly));
    source.write("[Desktop Entry]\nType=Application\nName=Fidan Shortcut Test\n"
                 "Icon=utilities-terminal\nExec=/bin/true\n");
    source.close();

    AppModel applications;
    applications.setFilter(QStringLiteral("Fidan Shortcut Test"));
    QCOMPARE(applications.rowCount(), 1);
    QVERIFY(applications.addToDesktop(0));
    const QFileInfo shortcut(targetPath);
    QVERIFY(shortcut.isFile());
    QVERIFY(shortcut.permission(QFileDevice::ExeOwner));

    DesktopModel desktop;
    bool found = false;
    for (int row = 0; row < desktop.rowCount(); ++row) {
        if (desktop.data(desktop.index(row), DesktopModel::NameRole).toString()
            == QStringLiteral("Fidan Shortcut Test")) {
            found = true;
            break;
        }
    }
    QVERIFY(found);
    QVERIFY(QFile::remove(sourcePath));
    QVERIFY(QFile::remove(targetPath));
}

void CoreTest::performsRealFileOperations()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    FilesModel files(directory.path());
    QCOMPARE(files.rowCount(), 0);

    QVERIFY(files.createFolder(QStringLiteral("Projects")));
    QVERIFY(files.createFile(QStringLiteral("notes.txt")));
    QCOMPARE(files.rowCount(), 2);
    QVERIFY(QFileInfo::exists(directory.filePath(QStringLiteral("Projects"))));
    QVERIFY(QFileInfo::exists(directory.filePath(QStringLiteral("notes.txt"))));

    int notesRow = -1;
    for (int row = 0; row < files.rowCount(); ++row) {
        if (files.entryName(row) == QStringLiteral("notes.txt")) {
            notesRow = row;
            break;
        }
    }
    QVERIFY(notesRow >= 0);
    QVERIFY(files.renameEntry(notesRow, QStringLiteral("renamed.txt")));
    QVERIFY(!QFileInfo::exists(directory.filePath(QStringLiteral("notes.txt"))));
    QVERIFY(QFileInfo::exists(directory.filePath(QStringLiteral("renamed.txt"))));

    int renamedRow = -1;
    for (int row = 0; row < files.rowCount(); ++row) {
        if (files.entryName(row) == QStringLiteral("renamed.txt")) {
            renamedRow = row;
            break;
        }
    }
    QVERIFY(renamedRow >= 0);
    QVERIFY(files.deletePermanently(renamedRow));
    QVERIFY(!QFileInfo::exists(directory.filePath(QStringLiteral("renamed.txt"))));
    QVERIFY(!files.createFile(QStringLiteral("../escape.txt")));
    QVERIFY(!QFileInfo::exists(QFileInfo(directory.path()).absoluteDir().filePath(QStringLiteral("escape.txt"))));
}

void CoreTest::rejectsMissingFolderWithoutChangingLocation()
{
    QTemporaryDir directory;
    FilesModel files(directory.path());
    files.setPath(directory.filePath(QStringLiteral("missing")));
    QCOMPARE(files.path(), directory.path());
    QVERIFY(!files.errorText().isEmpty());
    QVERIFY(files.createFolder(QStringLiteral("Project with spaces")));
    files.setPath(QUrl::fromLocalFile(directory.filePath(QStringLiteral("Project with spaces"))).toString());
    QCOMPARE(files.path(), directory.filePath(QStringLiteral("Project with spaces")));
}

void CoreTest::isolatesApplicationWindowEnvironment()
{
#ifdef Q_OS_UNIX
    QTemporaryDir directory;
    const QString output = directory.filePath(QStringLiteral("environment.txt"));
    const QByteArray previous = qgetenv("QT_WAYLAND_SHELL_INTEGRATION");
    qputenv("QT_WAYLAND_SHELL_INTEGRATION", "layer-shell");
    const bool started = startDesktopProcess(QStringLiteral("/bin/sh"),
        {QStringLiteral("-c"), QStringLiteral("printenv QT_WAYLAND_SHELL_INTEGRATION > \"$1\""), QStringLiteral("capture"), output});
    if (previous.isNull()) qunsetenv("QT_WAYLAND_SHELL_INTEGRATION");
    else qputenv("QT_WAYLAND_SHELL_INTEGRATION", previous);
    QVERIFY(started);
    QTRY_VERIFY(QFileInfo(output).size() > 0);
    QFile file(output);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll().trimmed(), QByteArray("xdg-shell"));
#endif
}

void CoreTest::synchronizesPreferencesAcrossApplications()
{
    ShellController desktop(true, true);
    ShellController settings(true, true);
    const bool originalIcons = desktop.desktopIconsVisible();
    const bool originalNotifications = desktop.notificationsEnabled();
    settings.setDesktopIconsVisible(!originalIcons);
    settings.setNotificationsEnabled(!originalNotifications);
    QTRY_COMPARE_WITH_TIMEOUT(desktop.desktopIconsVisible(), !originalIcons, 5000);
    QCOMPARE(desktop.notificationsEnabled(), !originalNotifications);
    settings.setDesktopIconsVisible(originalIcons);
    settings.setNotificationsEnabled(originalNotifications);
}

void CoreTest::readsRealMemoryMetrics()
{
#ifdef Q_OS_LINUX
    SystemMonitor monitor(false);
    QVERIFY(!monitor.memoryLabel().isEmpty());
    QVERIFY(monitor.memoryUsage() > 0 && monitor.memoryUsage() <= 100);
    ProcessModel processes;
    QVERIFY(processes.totalMemory() > 0 && processes.totalMemory() <= 100);
#endif
}

void CoreTest::opensDesktopFolderWithSpacesAndDots()
{
#ifdef Q_OS_UNIX
    QTemporaryDir tools;
    const QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QTemporaryDir folder(QDir(desktopPath).filePath(QStringLiteral("Fidan Project.XXXXXX")));
    QVERIFY(folder.isValid());
    const QString capture = tools.filePath(QStringLiteral("opened.txt"));
    QFile fakeManager(tools.filePath(QStringLiteral("fidan-files")));
    QVERIFY(fakeManager.open(QIODevice::WriteOnly));
    fakeManager.write("#!/bin/sh\nprintf '%s\\n' \"$1\" \"$QT_WAYLAND_SHELL_INTEGRATION\" > \"$FIDAN_OPEN_CAPTURE\"\n");
    fakeManager.close();
    QVERIFY(fakeManager.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner));
    DesktopModel desktop;
    int found = -1;
    for (int row = 0; row < desktop.rowCount(); ++row) {
        if (desktop.data(desktop.index(row), DesktopModel::PathRole).toString() == folder.path()) {
            found = row;
            QCOMPARE(desktop.data(desktop.index(row), DesktopModel::NameRole).toString(), QFileInfo(folder.path()).fileName());
            break;
        }
    }
    QVERIFY(found >= 0);
    const QByteArray oldPath = qgetenv("PATH"), oldCapture = qgetenv("FIDAN_OPEN_CAPTURE");
    qputenv("PATH", tools.path().toLocal8Bit() + ':' + oldPath);
    qputenv("FIDAN_OPEN_CAPTURE", capture.toLocal8Bit());
    const bool opened = desktop.open(found);
    qputenv("PATH", oldPath);
    if (oldCapture.isNull()) qunsetenv("FIDAN_OPEN_CAPTURE"); else qputenv("FIDAN_OPEN_CAPTURE", oldCapture);
    QVERIFY(opened);
    QTRY_VERIFY(QFileInfo(capture).size() > 0);
    QFile output(capture); QVERIFY(output.open(QIODevice::ReadOnly));
    const QList<QByteArray> lines = output.readAll().split('\n');
    QCOMPARE(QString::fromUtf8(lines.value(0)), folder.path());
    QCOMPARE(lines.value(1), QByteArray("xdg-shell"));
#endif
}

void CoreTest::opensTextDocumentWithSpacesInEditor()
{
#ifdef Q_OS_UNIX
    QTemporaryDir directory;
    const QString capture = directory.filePath(QStringLiteral("editor-arguments.txt"));
    QFile editor(directory.filePath(QStringLiteral("featherpad")));
    QVERIFY(editor.open(QIODevice::WriteOnly));
    editor.write("#!/bin/sh\nprintf '%s\\n' \"$1\" \"$QT_WAYLAND_SHELL_INTEGRATION\" > \"$FIDAN_EDITOR_CAPTURE\"\n");
    editor.close();
    QVERIFY(editor.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner));
    QFile mimeTool(directory.filePath(QStringLiteral("xdg-mime")));
    QVERIFY(mimeTool.open(QIODevice::WriteOnly)); mimeTool.write("#!/bin/sh\nexit 1\n"); mimeTool.close();
    QVERIFY(mimeTool.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner));
    const QString document = directory.filePath(QStringLiteral("New File.txt"));
    QFile file(document); QVERIFY(file.open(QIODevice::WriteOnly)); file.close();
    const QByteArray oldPath = qgetenv("PATH"), oldCapture = qgetenv("FIDAN_EDITOR_CAPTURE");
    qputenv("PATH", directory.path().toLocal8Bit() + ':' + oldPath);
    qputenv("FIDAN_EDITOR_CAPTURE", capture.toLocal8Bit());
    const bool opened = openDesktopFile(document);
    qputenv("PATH", oldPath);
    if (oldCapture.isNull()) qunsetenv("FIDAN_EDITOR_CAPTURE"); else qputenv("FIDAN_EDITOR_CAPTURE", oldCapture);
    QVERIFY(opened);
    QTRY_VERIFY(QFileInfo(capture).size() > 0);
    QFile output(capture); QVERIFY(output.open(QIODevice::ReadOnly));
    const auto lines = output.readAll().split('\n');
    QCOMPARE(QString::fromUtf8(lines.value(0)), document);
    QCOMPARE(lines.value(1), QByteArray("xdg-shell"));
    QVERIFY(!openDesktopFile(directory.filePath(QStringLiteral("missing.txt"))));
#endif
}

void CoreTest::sortsDesktopItems()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QVERIFY(QDir(directory.path()).mkdir(QStringLiteral("Projects")));
    for (const QString &name : {QStringLiteral("a.txt"), QStringLiteral("z.png")}) {
        QFile file(directory.filePath(name));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QVERIFY(file.setFileTime(QDateTime::fromSecsSinceEpoch(name == QStringLiteral("a.txt") ? 1000000000 : 1100000000), QFileDevice::FileModificationTime));
    }
    DesktopModel desktop(directory.path());
    QCOMPARE(desktop.rowCount(), 3);
    QCOMPARE(desktop.data(desktop.index(0), DesktopModel::NameRole).toString(), QStringLiteral("Projects"));
    QCOMPARE(desktop.data(desktop.index(1), DesktopModel::NameRole).toString(), QStringLiteral("a.txt"));
    desktop.setSortMode(QStringLiteral("Type"));
    QCOMPARE(desktop.data(desktop.index(1), DesktopModel::NameRole).toString(), QStringLiteral("z.png"));
    desktop.setSortMode(QStringLiteral("Date Modified"));
    QCOMPARE(desktop.data(desktop.index(0), DesktopModel::NameRole).toString(), QStringLiteral("Projects"));
    QCOMPARE(desktop.data(desktop.index(1), DesktopModel::NameRole).toString(), QStringLiteral("z.png"));
    desktop.setSortMode(QStringLiteral("invalid"));
    QCOMPARE(desktop.sortMode(), QStringLiteral("Date Modified"));
    desktop.setSortMode(QStringLiteral("Name"));
    QCOMPARE(desktop.data(desktop.index(1), DesktopModel::NameRole).toString(), QStringLiteral("a.txt"));
}

void CoreTest::clearsSearchWhenChangingFolders()
{
    QTemporaryDir directory;
    QVERIFY(QDir(directory.path()).mkdir(QStringLiteral("Projects")));
    FilesModel files(directory.path());
    files.setFilter(QStringLiteral("missing"));
    QCOMPARE(files.rowCount(), 0);
    files.setPath(directory.filePath(QStringLiteral("missing")));
    QCOMPARE(files.filter(), QStringLiteral("missing"));
    files.setPath(directory.filePath(QStringLiteral("Projects")));
    QCOMPARE(files.filter(), QString());
    QVERIFY(files.createFile(QStringLiteral("notes.txt")));
    QCOMPARE(files.rowCount(), 1);
    files.back();
    QCOMPARE(files.path(), directory.path());
    QCOMPARE(files.rowCount(), 1);
}

void CoreTest::keepsHistoryWhenFolderDisappears()
{
    QTemporaryDir directory;
    const QString first = directory.filePath(QStringLiteral("First"));
    const QString second = directory.filePath(QStringLiteral("Second"));
    QVERIFY(QDir().mkdir(first));
    QVERIFY(QDir().mkdir(second));
    FilesModel files(directory.path());
    files.setPath(first);
    files.setPath(second);
    QVERIFY(QDir().rmdir(first));
    files.back();
    QCOMPARE(files.path(), second);
    QVERIFY(files.canGoBack());
    QVERIFY(!files.canGoForward());
    QVERIFY(!files.errorText().isEmpty());
    QVERIFY(QDir().mkdir(first));
    files.back();
    QCOMPARE(files.path(), first);
    QVERIFY(files.canGoForward());
    QVERIFY(QDir().rmdir(second));
    files.forward();
    QCOMPARE(files.path(), first);
    QVERIFY(files.canGoForward());
    QVERIFY(QDir().mkdir(second));
    files.forward();
    QCOMPARE(files.path(), second);
    QVERIFY(files.errorText().isEmpty());
}

void CoreTest::measuresStorageWithoutCountingReservedBlocks()
{
#ifdef Q_OS_LINUX
    struct statvfs storage;
    QVERIFY(::statvfs("/", &storage) == 0);
    QVERIFY(storage.f_blocks > 0);
    const double expected = 100.0 * static_cast<double>(storage.f_blocks - storage.f_bfree)
        / static_cast<double>(storage.f_blocks);
    SystemMonitor monitor(false);
    QVERIFY(qAbs(monitor.storageUsage() - expected) < 0.2);
#endif
}

void CoreTest::rejectsFirstBootCredentialInjection()
{
    QVERIFY(FirstBootValidation::account("fidan", "Fidan", "long:valid password"));
    QVERIFY(FirstBootValidation::account("fidan", QString::fromUtf8("Çağrı"), "validpass"));
    QVERIFY(!FirstBootValidation::account("fidan", "Fidan", "validpass\nroot:injected"));
    QVERIFY(!FirstBootValidation::account("fidan", "Fidan", "validpass\rroot:injected"));
    QVERIFY(!FirstBootValidation::account("fidan", "Fidan", QStringLiteral("validpass") + QChar::Null));
    QVERIFY(!FirstBootValidation::account("fidan\n", "Fidan", "validpass"));
    QVERIFY(!FirstBootValidation::account("fidan", "Fake\nrecord", "validpass"));
    QVERIFY(!FirstBootValidation::account("fidan", "Fake:record", "validpass"));
}

void CoreTest::refusesUntrustedDesktopLaunchers()
{
#ifdef Q_OS_UNIX
    QTemporaryDir directory;
    const QString capture = directory.filePath("executed");
    const QString path = directory.filePath("invoice.pdf.desktop");
    QFile launcher(path); QVERIFY(launcher.open(QIODevice::WriteOnly));
    launcher.write(("[Desktop Entry]\nType=Application\nName=Invoice.pdf\nIcon=application-pdf\nExec=/usr/bin/touch " + capture + "\n").toUtf8()); launcher.close();
    QVERIFY(launcher.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner));
    DesktopModel desktop(directory.path()); QCOMPARE(desktop.rowCount(), 1);
    QVERIFY(!desktop.open(0)); QVERIFY(!QFile::exists(capture));
    QVERIFY(launcher.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner | QFileDevice::WriteOther));
    QVERIFY(!desktop.open(0)); QVERIFY(!QFile::exists(capture));
    QVERIFY(launcher.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner));
    const QString link = directory.filePath("shortcut.desktop"); QVERIFY(QFile::link(path, link));
    desktop.refresh();
    for (int row=0; row<desktop.rowCount(); ++row)
        if (desktop.data(desktop.index(row), DesktopModel::PathRole).toString() == link) QVERIFY(!desktop.open(row));
    QVERIFY(!QFile::exists(capture));
    for (int row=0; row<desktop.rowCount(); ++row)
        if (desktop.data(desktop.index(row), DesktopModel::PathRole).toString() == path) QVERIFY(desktop.open(row));
    QTRY_VERIFY(QFile::exists(capture));
#endif
}


QTEST_MAIN(CoreTest)
#include "tst_core.moc"
