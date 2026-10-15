// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick

Item {
    implicitWidth: 94
    implicitHeight: 58

    Rectangle {
        anchors.centerIn: parent
        width: parent.width * 0.82
        height: parent.height * 0.72
        radius: 6
        rotation: -8
        color: "#2C754D"
        border.width: 1
        border.color: "#63DE87"

        Rectangle { x: 25; y: 12; width: 25; height: 22; radius: 3; color: "#16221E"; border.color: "#83978D" }
        Rectangle { x: 55; y: 8; width: 17; height: 10; radius: 2; color: "#BCC4BF" }
        Rectangle { x: 57; y: 24; width: 18; height: 11; radius: 2; color: "#CDD1CF" }
        Rectangle { x: 4; y: 10; width: 13; height: 21; radius: 2; color: "#C8CECB" }
        Row {
            x: 18; y: 3; spacing: 2
            Repeater { model: 13; Rectangle { width: 2; height: 5; color: "#D5B44A" } }
        }
        Repeater {
            model: [{x:5,y:4},{x:68,y:4},{x:5,y:30},{x:68,y:30}]
            Rectangle { required property var modelData; x:modelData.x; y:modelData.y; width:4; height:4; radius:2; color:"#D9D9C8" }
        }
    }
}
