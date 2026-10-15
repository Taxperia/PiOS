// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import Fidan.Shell

Window {
    id: root
    width: Screen.width; height: Screen.height
    color: "#08110E"; flags: Qt.FramelessWindowHint
    property bool loading: true
    Component.onCompleted: {
        if (NestedSession && !Controller.previewMode) {
            LayerShell.configure(root, "startup")
            visible = true
        }
    }
    Timer { interval: 1800; running: root.visible && root.loading; onTriggered: root.loading = false }
    DesktopBackground { anchors.fill: parent; visible: !root.loading }
    Rectangle { anchors.fill: parent; visible: !root.loading; color: "#60040B08" }
    Column {
        anchors.centerIn: parent; width: 360; spacing: 20
        Text { anchors.horizontalCenter: parent.horizontalCenter; text: root.loading ? Product.name : Controller.clockText; color: root.loading ? Theme.accent : "white"; font.family: Theme.fontFamily; font.pixelSize: root.loading ? 38 : 68; font.weight: Font.Light }
        Text { anchors.horizontalCenter: parent.horizontalCenter; visible: !root.loading; text: Controller.dateText; color: "#D7E0DB"; font.family: Theme.fontFamily; font.pixelSize: 16 }
        BusyIndicator { anchors.horizontalCenter: parent.horizontalCenter; visible: root.loading; running: visible }
        Text { width: parent.width; horizontalAlignment: Text.AlignHCenter; text: root.loading ? qsTr("Starting desktop…") : qsTr("Debian development session"); color: "#B7C5BE"; font.family: Theme.fontFamily; font.pixelSize: 13 }
        Button {
            visible: !root.loading; width: parent.width; height: 42; text: qsTr("Enter desktop")
            onClicked: root.close()
            background: Rectangle { radius: 9; color: Theme.accent }
            contentItem: Text { text: parent.text; color: "#08110E"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; font.pixelSize: 13; font.weight: Font.DemiBold }
        }
        Text { visible: !root.loading; width: parent.width; text: qsTr("WSL development session · Your device uses the authenticated login screen."); wrapMode: Text.Wrap; horizontalAlignment: Text.AlignHCenter; color: "#B7C5BE"; font.pixelSize: 10 }
    }
    Shortcut { sequence: "Return"; enabled: !root.loading && root.visible; onActivated: root.close() }
}
