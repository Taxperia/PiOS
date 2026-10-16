// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import Fidan.Shell

Window {
    id: window
    property bool contextMenuVisible: false
    color: "transparent"
    flags: Qt.FramelessWindowHint
    Component.onCompleted: {
        LayerShell.configure(window, "background")
        visible = !Controller.previewMode
    }
    DesktopBackground { anchors.fill: parent }
    DesktopIcons {
        id: desktopIcons
        anchors.left: parent.left
        anchors.leftMargin: Controller.dockPosition === "left" ? 92 * Controller.interfaceScale : 22
        anchors.top: parent.top
        anchors.topMargin: Theme.panelHeight * Controller.interfaceScale + 18
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Controller.dockPosition === "bottom" ? 90 * Controller.interfaceScale : 10
        anchors.right: parent.right
        anchors.rightMargin: Controller.dockPosition === "right" ? 92 * Controller.interfaceScale : 22
        z: 2
    }
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        z: 1
        onPressed: mouse => {
            if (mouse.button !== Qt.RightButton)
                return
            desktopContextMenu.x = Math.max(8, Math.min(mouse.x, window.width - desktopContextMenu.width - 8))
            desktopContextMenu.y = Math.max(Theme.panelHeight + 8, Math.min(mouse.y, window.height - desktopContextMenu.height - 8))
            window.contextMenuVisible = true
            desktopContextMenu.forceActiveFocus()
            mouse.accepted = true
        }
    }
    MouseArea {
        anchors.fill: parent
        visible: window.contextMenuVisible
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        z: 8
        onPressed: mouse => {
            window.contextMenuVisible = false
            mouse.accepted = true
        }
    }
    DesktopContextMenu {
        id: desktopContextMenu
        visible: window.contextMenuVisible
        z: 9
        onDismissed: window.contextMenuVisible = false
        onSettingsPageRequested: page => Controller.openSettingsPage(page)
    }
    StatusCard {
        anchors.top: parent.top
        anchors.topMargin: 76
        anchors.right: parent.right
        anchors.rightMargin: 16
        visible: Controller.statusCardVisible
        z: 3
    }
}

