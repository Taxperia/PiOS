// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import Fidan.Shell

Window {
    id: window
    readonly property real popupScale: Math.min(Controller.interfaceScale,
        (Screen.height - 160) / 500, (Screen.width - 40) / 380)
    width: Math.round(380 * popupScale)
    height: Math.round(500 * popupScale)
    color: "transparent"
    flags: Qt.FramelessWindowHint
    Component.onCompleted: {
        LayerShell.configure(window, "notification-center")
        visible = !Controller.previewMode && Notifications.historyVisible
    }
    Connections {
        target: Notifications
        function onHistoryVisibleChanged() {
            window.visible = !Controller.previewMode && Notifications.historyVisible
        }
    }
    Item {
        width: 380; height: 500
        anchors.centerIn: parent; scale: window.popupScale
        NotificationCenter { anchors.fill: parent; anchors.margins: 8 }
    }
}
