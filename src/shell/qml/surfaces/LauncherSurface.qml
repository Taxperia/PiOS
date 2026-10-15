// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import Fidan.Shell

Window {
    id: window
    readonly property real popupScale: Math.min(Controller.interfaceScale,
        (Screen.height - 120) / 540, (Screen.width - 120) / 760)
    width: Math.round(760 * popupScale)
    height: Math.round(540 * popupScale)
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
        width: 760; height: 540
        anchors.centerIn: parent; scale: window.popupScale
        AppLauncher { anchors.fill: parent; anchors.margins: 8 }
    }
}

