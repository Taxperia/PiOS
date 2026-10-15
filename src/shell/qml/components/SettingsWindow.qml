// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import Fidan.Shell

GlassPanel {
    id: root
    implicitWidth: 760
    implicitHeight: 500
    raised: true
    radius: 14
    clip: true

    signal closeRequested
    signal minimizeRequested
    signal maximizeRequested

    property bool maximized: false
    property alias selectedCategory: settingsContent.selectedCategory
    property string updateText: qsTr("Check for Updates")
    readonly property var categories: [
        { name: "System", icon: "settings" }, { name: "Display", icon: "apps" },
        { name: "Network", icon: "wifi" }, { name: "Bluetooth", icon: "bluetooth" },
        { name: "Power", icon: "bolt" }, { name: "Keyboard & Mouse", icon: "apps" },
        { name: "Personalization", icon: "moon" }, { name: "Applications", icon: "grid" },
        { name: "Privacy", icon: "lock" }, { name: "About", icon: "board" }
    ]

    Rectangle {
        id: titleBar
        anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
        height: 42; color: "#E50F1715"

        MouseArea {
            anchors.left: parent.left; anchors.right: controls.left
            anchors.top: parent.top; anchors.bottom: parent.bottom
            cursorShape: root.maximized ? Qt.ArrowCursor : Qt.SizeAllCursor
            drag.target: root.maximized ? null : root
            drag.minimumX: 0
            drag.maximumX: Math.max(0, root.parent ? root.parent.width - root.width : 0)
            drag.minimumY: Theme.panelHeight
            drag.maximumY: Math.max(Theme.panelHeight, root.parent ? root.parent.height - root.height : Theme.panelHeight)
            onDoubleClicked: root.maximizeRequested()
        }

        Row {
            anchors.left: parent.left; anchors.leftMargin: 14; anchors.verticalCenter: parent.verticalCenter; spacing: 8
            SystemIcon { anchors.verticalCenter: parent.verticalCenter; width: 16; height: 16; name: "settings"; color: Theme.text(Controller.darkTheme) }
            Text { anchors.verticalCenter: parent.verticalCenter; text: qsTr("Settings"); color: Theme.text(Controller.darkTheme); font.family: Theme.fontFamily; font.pixelSize: 11; font.weight: Font.DemiBold }
        }

        Row {
            id: controls
            anchors.right: parent.right; anchors.top: parent.top; height: parent.height

            component WindowButton: Rectangle {
                id: control
                property string symbol: ""
                property bool destructive: false
                signal clicked
                width: 42; height: titleBar.height
                color: controlMouse.containsMouse ? (destructive ? "#C74646" : "#34413D") : "transparent"
                Text { anchors.centerIn: parent; text: control.symbol; color: Theme.text(Controller.darkTheme); font.family: "sans-serif"; font.pixelSize: 13 }
                MouseArea { id: controlMouse; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: control.clicked() }
            }

            WindowButton { symbol: "−"; onClicked: root.minimizeRequested() }
            WindowButton { symbol: root.maximized ? "❐" : "□"; onClicked: root.maximizeRequested() }
            WindowButton { symbol: "×"; destructive: true; onClicked: root.closeRequested() }
        }
    }

    SettingsContent {
        id: settingsContent
        anchors.left: parent.left; anchors.right: parent.right
        anchors.top: titleBar.bottom; anchors.bottom: parent.bottom
    }
}
