// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Window
import Fidan.Shell

Window {
    id: window
    readonly property real fitScale: Math.min(Controller.interfaceScale, dock.horizontal ? (Screen.width-40)/(dock.width+20) : (Screen.height-Theme.panelHeight*Controller.interfaceScale-28)/(dock.height+20))
    width: (dock.width+20) * fitScale
    height: (dock.height+20) * fitScale
    color: "transparent"
    flags: Qt.FramelessWindowHint
    Component.onCompleted: {
        LayerShell.configure(window, "dock")
        LayerShell.positionDock(window, Controller.dockPosition)
        visible = !Controller.previewMode
    }
    Connections { target: Controller; function onPreferencesChanged() { LayerShell.positionDock(window, Controller.dockPosition) } }
    Dock { id: dock; anchors.centerIn: parent; scale: window.fitScale }
}

