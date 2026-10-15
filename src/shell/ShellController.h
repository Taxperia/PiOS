// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantList>

class QTimer;
class QProcess;

class ShellController final : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.fidan.Shell1")
    Q_PROPERTY(QString wallpaperUrl READ wallpaperUrl WRITE setWallpaperUrl NOTIFY preferencesChanged)
    Q_PROPERTY(bool desktopIconsVisible READ desktopIconsVisible WRITE setDesktopIconsVisible NOTIFY preferencesChanged)
    Q_PROPERTY(bool notificationsEnabled READ notificationsEnabled WRITE setNotificationsEnabled NOTIFY preferencesChanged)
    Q_PROPERTY(QString keyboardLayout READ keyboardLayout WRITE setKeyboardLayout NOTIFY preferencesChanged)
    Q_PROPERTY(QString systemSummary READ systemSummary CONSTANT)
    Q_PROPERTY(bool previewMode READ previewMode CONSTANT)
    Q_PROPERTY(bool mockMode READ mockMode CONSTANT)
    Q_PROPERTY(bool darkTheme READ darkTheme WRITE setDarkTheme NOTIFY darkThemeChanged)
    Q_PROPERTY(QString themeMode READ themeMode WRITE setThemeMode NOTIFY preferencesChanged)
    Q_PROPERTY(QString accentColor READ accentColor WRITE setAccentColor NOTIFY preferencesChanged)
    Q_PROPERTY(QString dockPosition READ dockPosition WRITE setDockPosition NOTIFY preferencesChanged)
    Q_PROPERTY(int dockIconSize READ dockIconSize WRITE setDockIconSize NOTIFY preferencesChanged)
    Q_PROPERTY(QString lockWallpaperUrl READ lockWallpaperUrl WRITE setLockWallpaperUrl NOTIFY preferencesChanged)
    Q_PROPERTY(bool reducedMotion READ reducedMotion WRITE setReducedMotion NOTIFY reducedMotionChanged)
    Q_PROPERTY(double interfaceScale READ interfaceScale WRITE setInterfaceScale NOTIFY interfaceScaleChanged)
    Q_PROPERTY(bool quickSettingsVisible READ quickSettingsVisible WRITE setQuickSettingsVisible NOTIFY quickSettingsVisibleChanged)
    Q_PROPERTY(bool launcherVisible READ launcherVisible WRITE setLauncherVisible NOTIFY launcherVisibleChanged)
    Q_PROPERTY(bool settingsVisible READ settingsVisible NOTIFY settingsVisibleChanged)
    Q_PROPERTY(bool softwareCenterVisible READ softwareCenterVisible NOTIFY softwareCenterVisibleChanged)
    Q_PROPERTY(bool statusCardVisible READ statusCardVisible NOTIFY statusCardVisibleChanged)
    Q_PROPERTY(bool lockScreenVisible READ lockScreenVisible NOTIFY lockScreenVisibleChanged)
    Q_PROPERTY(bool firstBootVisible READ firstBootVisible NOTIFY firstBootVisibleChanged)
    Q_PROPERTY(bool wifiAvailable READ wifiAvailable NOTIFY connectivityChanged)
    Q_PROPERTY(bool wifiEnabled READ wifiEnabled NOTIFY connectivityChanged)
    Q_PROPERTY(bool bluetoothAvailable READ bluetoothAvailable NOTIFY connectivityChanged)
    Q_PROPERTY(bool bluetoothEnabled READ bluetoothEnabled NOTIFY connectivityChanged)
    Q_PROPERTY(bool audioAvailable READ audioAvailable NOTIFY audioAvailabilityChanged)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool volumeMuted READ volumeMuted NOTIFY volumeMutedChanged)
    Q_PROPERTY(bool brightnessAvailable READ brightnessAvailable NOTIFY brightnessAvailabilityChanged)
    Q_PROPERTY(int brightness READ brightness WRITE setBrightness NOTIFY brightnessChanged)
    Q_PROPERTY(int activeWorkspace READ activeWorkspace NOTIFY activeWorkspaceChanged)
    Q_PROPERTY(bool powerProfilesAvailable READ powerProfilesAvailable CONSTANT)
    Q_PROPERTY(bool powerActionsAvailable READ powerActionsAvailable CONSTANT)
    Q_PROPERTY(bool lockAvailable READ lockAvailable CONSTANT)
    Q_PROPERTY(QString powerProfile READ powerProfile NOTIFY powerProfileChanged)
    Q_PROPERTY(QStringList pinnedApps READ pinnedApps CONSTANT)
    Q_PROPERTY(QStringList runningApps READ runningApps NOTIFY runningAppsChanged)
    Q_PROPERTY(QString clockText READ clockText NOTIFY clockChanged)
    Q_PROPERTY(QString dateText READ dateText NOTIFY clockChanged)

public:
    explicit ShellController(bool previewMode, bool mockMode, QObject *parent = nullptr);

    QString wallpaperUrl() const { return m_wallpaperUrl; }
    void setWallpaperUrl(const QString &value);
    bool desktopIconsVisible() const { return m_desktopIconsVisible; }
    void setDesktopIconsVisible(bool value);
    bool notificationsEnabled() const { return m_notificationsEnabled; }
    void setNotificationsEnabled(bool value);
    QString keyboardLayout() const { return m_keyboardLayout; }
    void setKeyboardLayout(const QString &value);
    QString systemSummary() const;
    Q_INVOKABLE bool openSystemTool(const QString &tool);
    Q_INVOKABLE void openSettingsPage(const QString &page);
    bool previewMode() const { return m_previewMode; }
    bool mockMode() const { return m_mockMode; }
    bool darkTheme() const { return m_darkTheme; }
    void setDarkTheme(bool darkTheme);
    QString themeMode() const { return m_themeMode; }
    void setThemeMode(const QString &value);
    QString accentColor() const { return m_accentColor; }
    void setAccentColor(const QString &value);
    QString dockPosition() const { return m_dockPosition; }
    void setDockPosition(const QString &value);
    int dockIconSize() const { return m_dockIconSize; }
    void setDockIconSize(int value);
    QString lockWallpaperUrl() const { return m_lockWallpaperUrl; }
    void setLockWallpaperUrl(const QString &value);
    bool reducedMotion() const { return m_reducedMotion; }
    void setReducedMotion(bool reducedMotion);
    double interfaceScale() const { return m_interfaceScale; }
    void setInterfaceScale(double scale);
    bool quickSettingsVisible() const { return m_quickSettingsVisible; }
    void setQuickSettingsVisible(bool visible);
    bool launcherVisible() const { return m_launcherVisible; }
    void setLauncherVisible(bool visible);
    bool settingsVisible() const { return m_settingsVisible; }
    bool softwareCenterVisible() const { return m_softwareCenterVisible; }
    bool statusCardVisible() const { return m_statusCardVisible; }
    bool lockScreenVisible() const { return m_lockScreenVisible; }
    bool firstBootVisible() const { return m_firstBootVisible; }
    bool wifiAvailable() const { return m_wifiAvailable; }
    bool wifiEnabled() const { return m_wifiEnabled; }
    bool bluetoothAvailable() const { return m_bluetoothAvailable; }
    bool bluetoothEnabled() const { return m_bluetoothEnabled; }
    bool audioAvailable() const { return m_audioAvailable; }
    int volume() const { return m_volume; }
    void setVolume(int volume);
    bool volumeMuted() const { return m_volumeMuted; }
    bool brightnessAvailable() const { return m_brightnessAvailable; }
    int brightness() const { return m_brightness; }
    void setBrightness(int brightness);
    int activeWorkspace() const { return m_activeWorkspace; }
    bool powerProfilesAvailable() const { return m_powerProfilesAvailable; }
    bool powerActionsAvailable() const { return m_powerActionsAvailable; }
    bool lockAvailable() const { return m_lockAvailable; }
    QString powerProfile() const { return m_powerProfile; }
    QStringList pinnedApps() const { return m_pinnedApps; }
    QStringList runningApps() const { return m_runningApps; }
    QString clockText() const;
    QString dateText() const;

    Q_INVOKABLE void toggleQuickSettings();
    Q_INVOKABLE void toggleLauncher();
    Q_INVOKABLE void openFileManager();
    Q_INVOKABLE void openSettings();
    Q_INVOKABLE void closeSettings();
    Q_INVOKABLE void openSoftwareCenter();
    Q_INVOKABLE void closeSoftwareCenter();
    Q_INVOKABLE void openTerminal();
    Q_INVOKABLE void openTaskManager();
    Q_INVOKABLE void toggleVolumeMute();
    Q_INVOKABLE QString createDesktopFolder();
    Q_INVOKABLE QString createDesktopFile();
    Q_INVOKABLE bool openDesktopTerminal();
    Q_INVOKABLE void toggleStatusCard();
    Q_INVOKABLE void showFirstBoot();
    Q_INVOKABLE void completeFirstBootPreview();
    Q_INVOKABLE void unlockPreview();
    Q_INVOKABLE void toggleWifi();
    Q_INVOKABLE void toggleBluetooth();
    Q_INVOKABLE void activateWorkspace(int workspace);
    Q_INVOKABLE void lockSession();
    Q_INVOKABLE void requestPowerAction(const QString &action);
    Q_INVOKABLE void cyclePowerProfile();
    Q_INVOKABLE bool launchPinned(const QString &desktopId);
    Q_INVOKABLE bool focusRunningApp(const QString &appId);
    Q_INVOKABLE QVariantList applicationChoices(const QString &mime) const;
    Q_INVOKABLE QString defaultApplication(const QString &mime) const;
    Q_INVOKABLE bool setDefaultApplication(const QString &mime, const QString &desktopId);

public slots:
    void OpenFiles() { openFileManager(); }
    void OpenSettingsPage(const QString &page) { openSettingsPage(page); }
    void OpenSoftwareCenter() { openSoftwareCenter(); }
    void OpenTaskManager() { openTaskManager(); }
    void ToggleLauncher() { toggleLauncher(); }
    void ToggleQuickSettings() { toggleQuickSettings(); }
    void SetWorkspace(int workspace) { activateWorkspace(workspace); }

signals:
    void preferencesChanged();
    void darkThemeChanged();
    void reducedMotionChanged();
    void interfaceScaleChanged();
    void quickSettingsVisibleChanged();
    void launcherVisibleChanged();
    void settingsVisibleChanged();
    void softwareCenterVisibleChanged();
    void statusCardVisibleChanged();
    void lockScreenVisibleChanged();
    void firstBootVisibleChanged();
    void connectivityChanged();
    void audioAvailabilityChanged();
    void brightnessAvailabilityChanged();
    void volumeChanged();
    void volumeMutedChanged();
    void brightnessChanged();
    void activeWorkspaceChanged();
    void powerProfileChanged();
    void runningAppsChanged();
    void clockChanged();
    void actionFailed(const QString &message);

private:
    void saveSettings();
    void reloadPreferences();
    void refreshConnectivity();
    void refreshAudio();
    void refreshBrightness();
    void refreshRunningApps();
    bool setDbusProperty(const QString &service, const QString &path,
                         const QString &interface, const QString &property,
                         const QVariant &value);
    QString desktopPath() const;
    QString uniqueDesktopName(const QString &baseName, const QString &suffix) const;

    QString m_wallpaperUrl;
    QString m_keyboardLayout = QStringLiteral("us");
    bool m_desktopIconsVisible = true;
    bool m_notificationsEnabled = true;
    bool m_previewMode = false;
    bool m_mockMode = false;
    bool m_darkTheme = true;
    QString m_themeMode = QStringLiteral("dark");
    QString m_accentColor = QStringLiteral("#63de8a");
    QString m_dockPosition = QStringLiteral("left");
    QString m_lockWallpaperUrl;
    int m_dockIconSize = 24;
    bool m_reducedMotion = false;
    double m_interfaceScale = 1.25;
    bool m_quickSettingsVisible = false;
    bool m_launcherVisible = false;
    bool m_settingsVisible = false;
    bool m_softwareCenterVisible = false;
    bool m_statusCardVisible = false;
    bool m_lockScreenVisible = false;
    bool m_firstBootVisible = false;
    bool m_wifiAvailable = false;
    bool m_wifiEnabled = false;
    bool m_bluetoothAvailable = false;
    bool m_bluetoothEnabled = false;
    bool m_audioAvailable = false;
    int m_volume = 62;
    bool m_volumeMuted = false;
    bool m_brightnessAvailable = false;
    int m_brightness = 58;
    int m_activeWorkspace = 1;
    bool m_powerProfilesAvailable = false;
    bool m_powerActionsAvailable = false;
    bool m_lockAvailable = false;
    QString m_powerProfile = QStringLiteral("balanced");
    QStringList m_pinnedApps;
    QStringList m_runningApps;
    QTimer *m_clockTimer = nullptr;
    QTimer *m_connectivityTimer = nullptr;
    QTimer *m_windowTimer = nullptr;
    QProcess *m_windowQuery = nullptr;
};
