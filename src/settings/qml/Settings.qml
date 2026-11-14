// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

ApplicationWindow {
    id: root
    readonly property real bottomDockSpace: Controller.dockPosition === "bottom" ? (Controller.dockIconSize + 58) * Controller.interfaceScale + 20 : 0
    onBottomDockSpaceChanged: {
        if (visible && !maximized) {
            height = Math.max(minimumHeight, Math.min(650, Screen.height - 74 - bottomDockSpace))
            y = Math.max(60, (Screen.height - bottomDockSpace - height) / 2)
        }
    }
    width: Math.max(minimumWidth, Math.min(1140, Screen.width * 0.9))
    height: Math.max(minimumHeight, Math.min(650, Screen.height - 74 - bottomDockSpace))
    minimumWidth: 760
    minimumHeight: 500
    x: Math.max(78, (Screen.width - width) / 2)
    y: Math.max(60, (Screen.height - bottomDockSpace - height) / 2)
    visible: true
    title: qsTr("Fidan Ayarları")
    color: "transparent"
    flags: Qt.Window | Qt.FramelessWindowHint
    palette.window: Controller.darkTheme ? Controller.themeColor("#132A27", Controller.themeMode, Controller.darkTheme) : Controller.themeColor("#F5FAF7", Controller.themeMode, Controller.darkTheme)
    palette.base: Controller.darkTheme ? Controller.themeColor("#102420", Controller.themeMode, Controller.darkTheme) : Controller.themeColor("#FFFFFF", Controller.themeMode, Controller.darkTheme)
    palette.text: Controller.darkTheme ? Controller.themeColor("#F2F7F4", Controller.themeMode, Controller.darkTheme) : Controller.themeColor("#18261F", Controller.themeMode, Controller.darkTheme)
    palette.windowText: Controller.darkTheme ? Controller.themeColor("#F2F7F4", Controller.themeMode, Controller.darkTheme) : Controller.themeColor("#18261F", Controller.themeMode, Controller.darkTheme)
    palette.buttonText: Controller.darkTheme ? Controller.themeColor("#F2F7F4", Controller.themeMode, Controller.darkTheme) : Controller.themeColor("#18261F", Controller.themeMode, Controller.darkTheme)
    palette.button: Controller.darkTheme ? Controller.themeColor("#254238", Controller.themeMode, Controller.darkTheme) : Controller.themeColor("#E1ECE5", Controller.themeMode, Controller.darkTheme)
    palette.highlight: Controller.accentColor
    palette.highlightedText: Controller.themeColor("#071910", Controller.themeMode, Controller.darkTheme)

    readonly property color accent: Controller.themeColor("#63DE8A", Controller.themeMode, Controller.darkTheme)
    readonly property color foreground: Controller.darkTheme ? Controller.themeColor("#F2F7F4", Controller.themeMode, Controller.darkTheme) : Controller.themeColor("#18261F", Controller.themeMode, Controller.darkTheme)
    readonly property color secondary: Controller.darkTheme ? Controller.themeColor("#9FB2A8", Controller.themeMode, Controller.darkTheme) : Controller.themeColor("#586B60", Controller.themeMode, Controller.darkTheme)
    readonly property bool maximized: visibility === Window.Maximized
    property int page: 0
    readonly property var pages: [
        { name: qsTr("System"), icon: "computer" },
        { name: qsTr("Display"), icon: "video-display" },
        { name: qsTr("Network & Bluetooth"), icon: "network-wireless" },
        { name: qsTr("Sound"), icon: "audio-volume-high" },
        { name: qsTr("Personalization"), icon: "preferences-desktop-theme" },
        { name: qsTr("Applications"), icon: "system-software-install" },
        { name: qsTr("About"), icon: "help-about" }
    ]

    function toggleMaximized() { maximized ? showNormal() : showMaximized() }

    component WindowControl: Rectangle {
        id: wc
        property string symbol
        property bool destructive: false
        signal invoked
        width: 46; height: 42
        color: mouse.containsMouse ? (destructive ? Controller.themeColor("#C74646", Controller.themeMode, Controller.darkTheme) : (Controller.darkTheme ? Controller.themeColor("#304039", Controller.themeMode, Controller.darkTheme) : Controller.themeColor("#CEE1D5", Controller.themeMode, Controller.darkTheme))) : "transparent"
        Text { anchors.centerIn: parent; text: wc.symbol; color: wc.destructive && mouse.containsMouse ? "white" : root.foreground; font.pixelSize: 14 }
        MouseArea { id: mouse; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: wc.invoked() }
    }

    component FidanButton: Rectangle {
        id: button
        property string label
        property bool enabled: true
        signal invoked
        implicitWidth: buttonLabel.implicitWidth + 28
        implicitHeight: 34
        radius: 7
        opacity: enabled ? 1 : 0.45
        color: buttonMouse.containsMouse && enabled ? Controller.themeColor("#397D58", Controller.themeMode, Controller.darkTheme) : Controller.themeColor("#28533E", Controller.themeMode, Controller.darkTheme)
        border.width: 1; border.color: Controller.themeColor("#347551", Controller.themeMode, Controller.darkTheme)
        Text { id: buttonLabel; anchors.centerIn: parent; text: button.label; color: root.foreground; font.family: "Inter"; font.pixelSize: 10; font.weight: Font.DemiBold }
        MouseArea { id: buttonMouse; anchors.fill: parent; enabled: button.enabled; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: button.invoked() }
    }

    background: Rectangle {
        radius: root.maximized ? 0 : 13
        color: Controller.darkTheme ? Controller.themeColor("#0E1715", Controller.themeMode, Controller.darkTheme) : Controller.themeColor("#EFF5F1", Controller.themeMode, Controller.darkTheme)
        border.width: root.maximized ? 0 : 1
        border.color: Controller.darkTheme ? Controller.themeColor("#465750", Controller.themeMode, Controller.darkTheme) : Controller.themeColor("#D1DFD6", Controller.themeMode, Controller.darkTheme)
        clip: true
    }

    Rectangle {
        id: titleBar
        anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
        height: 42; color: Controller.darkTheme ? Controller.themeColor("#101A17", Controller.themeMode, Controller.darkTheme) : Controller.themeColor("#E5EEE8", Controller.themeMode, Controller.darkTheme)
        MouseArea {
            anchors.left: parent.left; anchors.right: controls.left; anchors.top: parent.top; anchors.bottom: parent.bottom
            cursorShape: root.maximized ? Qt.ArrowCursor : Qt.SizeAllCursor
            onPressed: root.startSystemMove()
            onDoubleClicked: root.toggleMaximized()
        }
        Row {
            anchors.left: parent.left; anchors.leftMargin: 14; anchors.verticalCenter: parent.verticalCenter; spacing: 9
            Image { width: 18; height: 18; source: "image://theme/preferences-system" + (Controller.darkTheme ? "?theme=dark" : "?theme=light"); sourceSize: Qt.size(22,22) }
            Text { text: qsTr("Ayarlar"); color: root.foreground; font.family: "Inter"; font.pixelSize: 12; font.weight: Font.DemiBold }
        }
        Row {
            id: controls
            anchors.right: parent.right; anchors.top: parent.top
            WindowControl { symbol: "−"; onInvoked: root.showMinimized() }
            WindowControl { symbol: root.maximized ? "❐" : "□"; onInvoked: root.toggleMaximized() }
            WindowControl { symbol: "×"; destructive: true; onInvoked: root.close() }
        }
    }

    SettingsContent {
        anchors.left: parent.left; anchors.right: parent.right
        anchors.top: titleBar.bottom; anchors.bottom: parent.bottom
        selectedCategory: InitialSettingsPage
    }
    MouseArea { anchors.left: parent.left; anchors.top: titleBar.bottom; anchors.bottom: parent.bottom; width: 6; visible: !root.maximized; cursorShape: Qt.SizeHorCursor; onPressed: root.startSystemResize(Qt.LeftEdge) }
    MouseArea { anchors.right: parent.right; anchors.top: titleBar.bottom; anchors.bottom: parent.bottom; width: 6; visible: !root.maximized; cursorShape: Qt.SizeHorCursor; onPressed: root.startSystemResize(Qt.RightEdge) }
    MouseArea { anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; height: 6; visible: !root.maximized; cursorShape: Qt.SizeVerCursor; onPressed: root.startSystemResize(Qt.BottomEdge) }
}
