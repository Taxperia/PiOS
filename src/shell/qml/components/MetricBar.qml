// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import Fidan.Shell

Column {
    id: root
    property string title: ""
    property real value: 0
    property string valueText: Math.round(value) + "%"
    spacing: 6

    Row {
        width: root.width
        Text {
            width: parent.width - valueLabel.width
            text: root.title
            color: Theme.text(Controller.darkTheme)
            font.family: Theme.fontFamily
            font.pixelSize: 12
            font.weight: Font.DemiBold
        }
        Text {
            id: valueLabel
            text: root.valueText
            color: Theme.muted(Controller.darkTheme)
            font.family: Theme.fontFamily
            font.pixelSize: 11
        }
    }

    Rectangle {
        width: root.width
        height: 6
        radius: 3
        color: Controller.darkTheme ? "#394640" : "#CBD7D0"
        Rectangle {
            width: parent.width * Math.max(0, Math.min(100, root.value)) / 100
            height: parent.height
            radius: parent.radius
            color: Theme.accent
            Behavior on width { NumberAnimation { duration: Theme.motionNormal } }
        }
    }
}

