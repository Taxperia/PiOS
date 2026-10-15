// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import Fidan.Shell

Rectangle {
    id: panel
    property bool raised: false
    property bool interactive: true
    color: raised ? Theme.raised(Controller.darkTheme) : Theme.surface(Controller.darkTheme)
    border.width: 1
    border.color: Theme.border(Controller.darkTheme)
    radius: Theme.radiusLarge
    antialiasing: true

    Rectangle {
        anchors.fill: parent
        anchors.margins: 1
        radius: Math.max(0, parent.radius - 1)
        color: "transparent"
        border.width: panel.raised ? 1 : 0
        border.color: Controller.darkTheme ? "#203A4841" : "#30FFFFFF"
    }
}

