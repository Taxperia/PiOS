// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import Fidan.Shell

GridView {
    id: root
    readonly property real interfaceScale: Controller.previewMode ? 1 : Controller.interfaceScale
    cellWidth: 96 * interfaceScale
    cellHeight: 96 * interfaceScale
    model: DesktopItems
    visible: Controller.desktopIconsVisible
    clip: true
    flow: GridView.FlowTopToBottom

    delegate: Rectangle {
        required property int index
        required property string name
        required property string iconName
        required property bool isDirectory
        width: 88
        height: 88
        scale: root.interfaceScale
        transformOrigin: Item.TopLeft
        radius: 9
        color: iconMouse.containsMouse ? "#602B3A34" : "transparent"
        Column {
            anchors.centerIn: parent
            width: parent.width - 8
            spacing: 5
            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                width: 44; height: 44; radius: 10
                color: isDirectory ? "#315F46" : "#315F46"
                SystemIcon { anchors.centerIn: parent; width: 27; height: 27; visible: isDirectory; name: "folder"; color: "white" }
                Image {
                    anchors.fill: parent
                    anchors.margins: 5
                    visible: !isDirectory
                    source: "image://theme/" + encodeURIComponent(iconName) + (Controller.darkTheme ? "?theme=dark" : "?theme=light")
                    sourceSize: Qt.size(36, 36)
                    fillMode: Image.PreserveAspectFit
                    asynchronous: true
                }
            }
            Text {
                width: parent.width
                text: name
                color: "white"
                font.family: Theme.fontFamily
                font.pixelSize: 10
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
                style: Text.Outline
                styleColor: "#A0000000"
            }
        }
        MouseArea {
            id: iconMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onDoubleClicked: {
                if (!DesktopItems.open(index))
                    Notifications.Notify("Files", 0, "dialog-warning", qsTr("Could not open item"), name, [], {}, 7000)
            }
        }
    }
}
