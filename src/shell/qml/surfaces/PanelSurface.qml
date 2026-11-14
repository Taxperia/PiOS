// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import Fidan.Shell

Window {
    id: window
    width: 1280
    height: Theme.panelHeight * Controller.interfaceScale
    color: "transparent"
    flags: Qt.FramelessWindowHint
    Component.onCompleted: {
        LayerShell.configure(window, "panel")
        LayerShell.positionPanel(window, Controller.panelPosition)
        visible = !Controller.previewMode
    }
    Connections { target: Controller; function onPreferencesChanged() { LayerShell.positionPanel(window, Controller.panelPosition) } }
    onHeightChanged: LayerShell.positionPanel(window, Controller.panelPosition)
    TopBar { width: window.width / Controller.interfaceScale; height: Theme.panelHeight; scale: Controller.interfaceScale; transformOrigin: Item.TopLeft }
}

