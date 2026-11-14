// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import Fidan.Shell

Window {
    id: window
    readonly property real popupScale: Math.min(Controller.interfaceScale,
        (Screen.height - 100) / 736, (Screen.width - 48) / 816)
    width: Math.round(816 * popupScale)
    height: Math.round(736 * popupScale)
    color: "transparent"
    flags: Qt.FramelessWindowHint

    Component.onCompleted: {
        LayerShell.configure(window, "launcher")
        visible = !Controller.previewMode && Controller.launcherVisible
    }
    Connections {
        target: Controller
        function onLauncherVisibleChanged() {
            window.visible = !Controller.previewMode && Controller.launcherVisible
        }
    }
    Item {
        width: 816; height: 736
        anchors.centerIn: parent; scale: window.popupScale
        AppLauncher { anchors.fill: parent; anchors.margins: 8 }
    }
}

