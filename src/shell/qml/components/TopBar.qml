// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Fidan.Shell

GlassPanel {
    id: root
    radius: 0; clip: true
    color: Controller.darkTheme ? "#F20D1816" : "#F2EDF4F0"
    RowLayout {
        anchors.fill: parent; anchors.leftMargin: 10; anchors.rightMargin: 10; spacing: 8
        IconButton { iconName: "menu"; compact: true; accessibleName: qsTr("Applications"); onClicked: Controller.toggleLauncher() }
        Row {
            spacing: 3; visible: root.width > 560
            Repeater {
                model: 3
                IconButton { required property int index; compact: true; label: String(index + 1); active: Controller.activeWorkspace === index + 1; onClicked: Controller.activateWorkspace(index + 1) }
            }
        }
        ListView {
            id: tasks
            Layout.fillWidth: true; Layout.minimumWidth: 0; Layout.preferredHeight: 34
            orientation: ListView.Horizontal; spacing: 5; clip: true
            model: Controller.runningApps
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.horizontal: ScrollBar { height: 3 }
            delegate: Rectangle {
                required property string modelData
                width: root.width > 950 ? 126 : 92; height: 32; radius: 6
                color: taskMouse.containsMouse
                    ? (Controller.darkTheme ? "#315F46" : "#C7E5D3")
                    : (Controller.darkTheme ? "#23312B" : "#DEEAE2")
                Text {
                    anchors.fill: parent; anchors.margins: 9
                    text: modelData.replace("fidan-", "").replace("org.fidan.", "")
                    color: Theme.text(Controller.darkTheme); font.family: Theme.fontFamily; font.pixelSize: 10
                    verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight
                }
                MouseArea { id: taskMouse; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: Controller.focusRunningApp(modelData) }
                ToolTip.visible: taskMouse.containsMouse; ToolTip.text: modelData
            }
        }
        Text {
            visible: root.width > 1060
            text: "CPU " + Math.round(SystemMonitor.cpuUsage) + "%  ·  RAM " + Math.round(SystemMonitor.memoryUsage) + "%"
            color: Theme.muted(Controller.darkTheme); font.family: Theme.fontFamily; font.pixelSize: 10
            TapHandler { onTapped: Controller.toggleStatusCard() }
        }
        IconButton { visible: root.width > 700; compact: true; iconName: "wifi"; iconColor: Controller.wifiEnabled ? Theme.accent : Theme.muted(Controller.darkTheme); accessibleName: qsTr("Network"); onClicked: Controller.toggleQuickSettings() }
        IconButton { visible: root.width > 850; compact: true; iconName: "bluetooth"; iconColor: Controller.bluetoothEnabled ? Theme.accent : Theme.muted(Controller.darkTheme); accessibleName: qsTr("Bluetooth"); onClicked: Controller.toggleQuickSettings() }
        IconButton { compact: true; iconName: "bell"; active: Notifications.historyVisible; accessibleName: qsTr("Notifications"); onClicked: Notifications.toggleHistory() }
        Column {
            Layout.alignment: Qt.AlignVCenter; spacing: 1
            Text { anchors.horizontalCenter: parent.horizontalCenter; text: Controller.clockText; color: Theme.text(Controller.darkTheme); font.family: Theme.fontFamily; font.pixelSize: 12; font.weight: Font.DemiBold }
            Text { visible: root.width > 700; text: Controller.dateText; color: Theme.muted(Controller.darkTheme); font.family: Theme.fontFamily; font.pixelSize: 9 }
        }
        IconButton { compact: true; iconName: "apps"; accessibleName: qsTr("Quick settings"); active: Controller.quickSettingsVisible; onClicked: Controller.toggleQuickSettings() }
    }
}
