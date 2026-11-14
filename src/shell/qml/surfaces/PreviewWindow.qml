// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import Fidan.Shell

Window {
    id: root
    property bool settingsMaximized: false
    property bool settingsMinimized: false
    property bool softwareMinimized: false
    property int activeWindow: 0
    property bool booting: !PreviewCapture
    Timer { interval: 1500; running: root.visible && root.booting; onTriggered: { root.booting = false; Controller.lockSession() } }
    property bool softwareCenterMaximized: false
    property bool contextMenuVisible: PreviewContextMenu
    property real settingsRestoreX: 350
    property real settingsRestoreY: 112
    property real softwareCenterRestoreX: 245
    property real softwareCenterRestoreY: 82
    width: 1440
    height: 810
    x: 0
    y: 0
    minimumWidth: 960
    minimumHeight: 540
    visible: Controller.previewMode
    color: Theme.darkBackground
    title: Product.name + " — Desktop Preview"

    Shortcut { sequence: "Ctrl+Alt+T"; onActivated: Controller.openTerminal() }
    Shortcut { sequences: ["Volume Up", "Ctrl+Alt+Up"]; onActivated: Controller.increaseVolume() }
    Shortcut { sequences: ["Volume Down", "Ctrl+Alt+Down"]; onActivated: Controller.decreaseVolume() }
    Shortcut { sequences: ["Volume Mute", "Ctrl+Alt+M"]; onActivated: Controller.toggleVolumeMute() }
    Shortcut { sequences: ["Brightness Adjust Up", "Ctrl+Alt+PgUp"]; onActivated: Controller.increaseBrightness() }
    Shortcut { sequences: ["Brightness Adjust Down", "Ctrl+Alt+PgDown"]; onActivated: Controller.decreaseBrightness() }
    Shortcut { sequence: "Ctrl+Alt+Shift+M"; onActivated: DesktopServices.toggleMicrophoneMute() }
    Shortcut { sequence: "Print"; onActivated: DesktopServices.screenshotVisible = true }

    Item {
        id: stage
        width: root.width / Controller.interfaceScale
        height: root.height / Controller.interfaceScale
        transformOrigin: Item.TopLeft
        scale: Controller.interfaceScale

        DesktopBackground { anchors.fill: parent }
        DesktopIcons {
            anchors.left: parent.left
            anchors.leftMargin: 84
            anchors.top: parent.top
            anchors.topMargin: Theme.panelHeight + 18
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.rightMargin: 22
            z: 2
        }
        MouseArea {
            id: desktopPointerArea
            anchors.fill: parent
            acceptedButtons: Qt.RightButton
            z: 1
            onPressed: mouse => {
                if (mouse.button !== Qt.RightButton)
                    return
                desktopContextMenu.x = Math.max(8, Math.min(mouse.x, stage.width - desktopContextMenu.width - 8))
                desktopContextMenu.y = Math.max(Theme.panelHeight + 8, Math.min(mouse.y, stage.height - desktopContextMenu.height - 8))
                root.contextMenuVisible = true
                desktopContextMenu.forceActiveFocus()
                mouse.accepted = true
            }
        }
        TopBar { anchors.left: parent.left; anchors.right: parent.right; y: Controller.panelPosition === "bottom" ? parent.height-height : 0; height: Theme.panelHeight; z: 10 }
        Rectangle {
            anchors.top: parent.top
            anchors.topMargin: Theme.panelHeight + 10
            anchors.horizontalCenter: parent.horizontalCenter
            width: 330
            height: 28
            radius: 8
            color: Controller.themeColor("#D90B1310", Controller.themeMode, Controller.darkTheme)
            border.width: 1
            border.color: Controller.themeColor("#52675D", Controller.themeMode, Controller.darkTheme)
            z: 12
            Text {
                anchors.centerIn: parent
                text: qsTr("VISUAL PREVIEW — system actions are simulated")
                color: Controller.themeColor("#D8E8DF", Controller.themeMode, Controller.darkTheme)
                font.family: Theme.fontFamily
                font.pixelSize: 9
                font.weight: Font.DemiBold
            }
        }
        Dock { x: Controller.dockPosition === "right" ? parent.width-width-14 : Controller.dockPosition === "bottom" ? (parent.width-width)/2 : 14; y: Controller.dockPosition === "bottom" ? parent.height-height-14 : (parent.height-height)/2; z: 4 }
        SettingsWindow {
            id: settingsWindow
            x: 350
            y: 112
            width: root.settingsMaximized ? stage.width - 88 : 760
            height: root.settingsMaximized ? stage.height - Theme.panelHeight - 28 : 500
            visible: Controller.settingsVisible && !root.settingsMinimized
            maximized: root.settingsMaximized
            onCloseRequested: {
                if (root.settingsMaximized) {
                    root.settingsMaximized = false
                    x = root.settingsRestoreX
                    y = root.settingsRestoreY
                }
                Controller.closeSettings()
            }
            onMinimizeRequested: root.settingsMinimized = true
            onMaximizeRequested: {
                if (!root.settingsMaximized) {
                    root.settingsRestoreX = x
                    root.settingsRestoreY = y
                    root.settingsMaximized = true
                    x = 72
                    y = Theme.panelHeight + 14
                } else {
                    root.settingsMaximized = false
                    x = root.settingsRestoreX
                    y = root.settingsRestoreY
                }
            }
            z: root.activeWindow === 0 ? 7 : 5
            TapHandler { gesturePolicy: TapHandler.WithinBounds; onPressedChanged: if (pressed) root.activeWindow = 0 }
        }
        SoftwareCenterWindow {
            id: softwareCenterWindow
            x: 245
            y: 82
            width: root.softwareCenterMaximized ? stage.width - 88 : 820
            height: root.softwareCenterMaximized ? stage.height - Theme.panelHeight - 28 : 530
            visible: Controller.softwareCenterVisible && !root.softwareMinimized
            maximized: root.softwareCenterMaximized
            onCloseRequested: {
                if (root.softwareCenterMaximized) {
                    root.softwareCenterMaximized = false
                    x = root.softwareCenterRestoreX
                    y = root.softwareCenterRestoreY
                }
                Controller.closeSoftwareCenter()
            }
            onMinimizeRequested: root.softwareMinimized = true
            onMaximizeRequested: {
                if (!root.softwareCenterMaximized) {
                    root.softwareCenterRestoreX = x
                    root.softwareCenterRestoreY = y
                    root.softwareCenterMaximized = true
                    x = 72
                    y = Theme.panelHeight + 14
                } else {
                    root.softwareCenterMaximized = false
                    x = root.softwareCenterRestoreX
                    y = root.softwareCenterRestoreY
                }
            }
            z: root.activeWindow === 1 ? 7 : 6
            TapHandler { gesturePolicy: TapHandler.WithinBounds; onPressedChanged: if (pressed) root.activeWindow = 1 }
        }
        StatusCard { anchors.top: parent.top; anchors.topMargin: 76; anchors.right: parent.right; anchors.rightMargin: 16; visible: Controller.statusCardVisible; z: 15 }
        QuickSettings {
            maximumHeight: stage.height - 94
            anchors.top: parent.top
            anchors.topMargin: 76
            anchors.right: parent.right
            anchors.rightMargin: 16
            visible: Controller.quickSettingsVisible
            z: 15
        }
        AppLauncher {
            anchors.centerIn: parent; visible: Controller.launcherVisible; z: 20
            scale: Math.min(1, (stage.width - 40) / implicitWidth, (stage.height - 100) / implicitHeight)
        }
        MouseArea {
            anchors.fill: parent
            visible: root.contextMenuVisible
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            z: 29
            onPressed: mouse => {
                root.contextMenuVisible = false
                mouse.accepted = true
            }
        }
        DesktopContextMenu {
            id: desktopContextMenu
            x: 190
            y: 150
            visible: root.contextMenuVisible
            z: 30
            onDismissed: root.contextMenuVisible = false
            onSettingsPageRequested: page => {
                settingsWindow.selectedCategory = page
                Controller.openSettings()
            }
        }
        NotificationToast {
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 18
            visible: Controller.notificationsEnabled && Notifications.visible && !Notifications.historyVisible
        }
        NotificationCenter {
            anchors.top: parent.top
            anchors.topMargin: 58
            anchors.right: parent.right
            anchors.rightMargin: 16
            visible: Notifications.historyVisible
        }
        Row {
            anchors.bottom: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter; anchors.bottomMargin: 12
            spacing: 8; z: 25
            Button { visible: Controller.settingsVisible; text: qsTr("Settings"); onClicked: { root.settingsMinimized = false; root.activeWindow = 0 } }
            Button { visible: Controller.softwareCenterVisible; text: qsTr("Software Center"); onClicked: { root.softwareMinimized = false; root.activeWindow = 1 } }
        }
        Rectangle {
            anchors.fill: parent; color: Controller.themeColor("#08110E", Controller.themeMode, Controller.darkTheme); visible: root.booting; z: 120
            Column {
                anchors.centerIn: parent; spacing: 25
                Text { anchors.horizontalCenter: parent.horizontalCenter; text: Product.name; color: Controller.themeColor("#63DE8A", Controller.themeMode, Controller.darkTheme); font.family: Theme.fontFamily; font.pixelSize: 38; font.weight: Font.Bold }
                BusyIndicator { anchors.horizontalCenter: parent.horizontalCenter; running: root.booting }
                Text { anchors.horizontalCenter: parent.horizontalCenter; text: qsTr("Starting desktop…"); color: Controller.themeColor("#9FB2A8", Controller.themeMode, Controller.darkTheme); font.pixelSize: 13 }
            }
        }
        LockScreenPreview {
            anchors.fill: parent
            visible: Controller.lockScreenVisible && !Controller.firstBootVisible
            z: 100
        }
        VolumeOsd {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: Controller.dockPosition === "bottom" ? 100 : 44
            visible: Controller.volumeOsdVisible
            z: 115
        }
        ScreenshotToolbar {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom; anchors.bottomMargin: 40
            visible: DesktopServices.screenshotVisible
            activeShortcuts: root.visible && visible
            z: 116
        }
        Loader {
            anchors.centerIn: parent
            width: Math.min(920,parent.width-32);height: Math.min(700,parent.height-48)
            visible: Controller.firstBootVisible
            active: visible
            sourceComponent: Controller.setupSession ? deviceWelcome : personalWelcome
            z: 110
        }
        Component { id: deviceWelcome; DeviceWelcomeWizard {} }
        Component { id: personalWelcome; WelcomeWizard {} }
    }

}

