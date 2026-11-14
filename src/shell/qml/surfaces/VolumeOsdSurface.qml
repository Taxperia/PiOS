// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import Fidan.Shell

Window {
    id: window
    readonly property real popupScale: Math.min(Controller.interfaceScale, (Screen.width - 32) / 316)
    width: Math.round(316 * popupScale)
    height: Math.round(104 * popupScale)
    color: "transparent"
    flags: Qt.FramelessWindowHint | Qt.WindowDoesNotAcceptFocus | Qt.WindowTransparentForInput

    Component.onCompleted: {
        LayerShell.configure(window, "volume-osd")
        visible = !Controller.previewMode && Controller.volumeOsdVisible
    }
    Connections {
        target: Controller
        function onVolumeOsdVisibleChanged() {
            window.visible = !Controller.previewMode && Controller.volumeOsdVisible
        }
    }
    VolumeOsd {
        anchors.centerIn: parent
        width: implicitWidth; height: implicitHeight
        scale: window.popupScale
    }
}
