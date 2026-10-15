// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import Fidan.Shell

Window {
    id: window
    width: 356
    height: toast.implicitHeight + 16
    color: "transparent"
    flags: Qt.FramelessWindowHint
    Component.onCompleted: {
        LayerShell.configure(window, "notification")
        visible = !Controller.previewMode && Controller.notificationsEnabled && Notifications.visible
    }
    Connections {
        target: Notifications
        function onCurrentChanged() { window.visible = !Controller.previewMode && Controller.notificationsEnabled && Notifications.visible }
    }
    Connections { target: Controller; function onPreferencesChanged() { window.visible = !Controller.previewMode && Controller.notificationsEnabled && Notifications.visible } }
    NotificationToast { id: toast; anchors.fill: parent; anchors.margins: 8 }
}
