// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import Fidan.Shell

QtObject {
    property StartupSurface startup: StartupSurface {}
    property WelcomeSurface welcome: WelcomeSurface {}
    property DesktopSurface desktop: DesktopSurface {}
    property PanelSurface panel: PanelSurface {}
    property DockSurface dock: DockSurface {}
    property QuickSettingsSurface quickSettings: QuickSettingsSurface {}
    property VolumeOsdSurface volumeOsd: VolumeOsdSurface {}
    property ScreenshotSurface screenshot: ScreenshotSurface {}
    property LauncherSurface launcher: LauncherSurface {}
    property NotificationSurface notifications: NotificationSurface {}
    property NotificationCenterSurface notificationCenter: NotificationCenterSurface {}
    property PreviewWindow preview: PreviewWindow {}
}

