// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import Fidan.Shell

GlassPanel {
    id: root
    implicitWidth: 280
    implicitHeight: 222
    raised: true
    radius: 15

    Column {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 9

        Item {
            width: parent.width; height: 52
            Column {
                anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter; spacing: 3
                Text { text: SystemMonitor.deviceModel; color: Theme.text(Controller.darkTheme); font.family: Theme.fontFamily; font.pixelSize: 14; font.weight: Font.Bold }
                Text { text: qsTr("Running smoothly"); color: Theme.muted(Controller.darkTheme); font.family: Theme.fontFamily; font.pixelSize: 10 }
            }
            PiBoard { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; width: 90; height: 52 }
        }

        MetricBar { width: parent.width; title: qsTr("CPU"); value: SystemMonitor.cpuUsage }
        MetricBar { width: parent.width; title: qsTr("RAM"); value: SystemMonitor.memoryUsage; valueText: SystemMonitor.memoryLabel }

        Rectangle { width: parent.width; height: 1; color: Theme.border(Controller.darkTheme) }
        Row {
            width: parent.width
            Row {
                width: parent.width * 0.55; spacing: 6
                SystemIcon { width: 13; height: 13; name: "temperature"; color: Theme.muted(Controller.darkTheme) }
                Text { text: qsTr("Temperature"); color: Theme.muted(Controller.darkTheme); font.family: Theme.fontFamily; font.pixelSize: 10 }
            }
            Text { width: parent.width * 0.45; horizontalAlignment: Text.AlignRight; text: SystemMonitor.temperatureAvailable ? Math.round(SystemMonitor.temperature) + "°C" : qsTr("Unavailable"); color: Theme.text(Controller.darkTheme); font.family: Theme.fontFamily; font.pixelSize: 10 }
        }
        MetricBar { width: parent.width; title: qsTr("Storage"); value: SystemMonitor.storageUsage; valueText: Math.round(SystemMonitor.storageUsage) + "%" }
    }
}
