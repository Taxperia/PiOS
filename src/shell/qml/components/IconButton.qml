// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import Fidan.Shell

Rectangle {
    id: root
    property string label: ""
    property string iconName: ""
    property color iconColor: active ? "white" : Theme.text(Controller.darkTheme)
    property string accessibleName: label
    property bool active: false
    property bool compact: false
    property int visualIconSize: compact ? 16 : 22
    signal clicked
    activeFocusOnTab: true
    Accessible.role: Accessible.Button
    Accessible.name: accessibleName

    implicitWidth: compact ? 30 : 42
    implicitHeight: compact ? 30 : 42
    radius: compact ? 8 : 10
    color: active ? Theme.accentStrong : (mouse.containsMouse ? "#3B45504B" : "transparent")
    border.width: active ? 1 : (activeFocus ? 2 : 0)
    border.color: Theme.accent

    Text {
        visible: root.iconName.length === 0
        anchors.centerIn: parent
        text: root.label
        color: root.active ? "white" : Theme.text(Controller.darkTheme)
        font.family: Theme.fontFamily
        font.pixelSize: root.compact ? 15 : 20
        font.weight: Font.Medium
    }

    SystemIcon {
        visible: root.iconName.length > 0
        anchors.centerIn: parent
        width: root.visualIconSize
        height: width
        name: root.iconName
        color: root.iconColor
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_Space) {
            root.clicked()
            event.accepted = true
        }
    }

    Behavior on color { ColorAnimation { duration: Theme.motionFast } }
}

