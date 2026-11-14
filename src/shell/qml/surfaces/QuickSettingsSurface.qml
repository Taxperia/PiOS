// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import Fidan.Shell

Window {
    id: window
    readonly property real popupScale: Math.min(Controller.interfaceScale,
        (Screen.height - 160) / content.height, (Screen.width - 40) / content.width)
    width: Math.round(content.width * popupScale)
    height: Math.round(content.height * popupScale)
    color: "transparent"
    flags: Qt.FramelessWindowHint

    Component.onCompleted: {
        LayerShell.configure(window, "quick-settings")
        visible = !Controller.previewMode && Controller.quickSettingsVisible
    }
    Connections {
        target: Controller
        function onQuickSettingsVisibleChanged() {
            window.visible = !Controller.previewMode && Controller.quickSettingsVisible
        }
    }
    Item {
        id: content
        width: 336; height: quickSettings.implicitHeight + 16
        anchors.centerIn: parent; scale: window.popupScale
        QuickSettings { id: quickSettings; anchors.fill: parent; anchors.margins: 8; maximumHeight: Math.max(300, (Screen.height - 110) / Controller.interfaceScale) }
    }
}

