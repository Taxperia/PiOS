// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import Fidan.Shell

Window {
    id: window
    readonly property real popupScale: Math.min(Controller.interfaceScale, (Screen.width - 32) / 636)
    width: Math.round(636 * popupScale)
    height: Math.round(186 * popupScale)
    color: "transparent"
    flags: Qt.FramelessWindowHint
    Component.onCompleted: {
        LayerShell.configure(window, "screenshot")
        visible = !Controller.previewMode && DesktopServices.screenshotVisible
    }
    Connections {
        target: DesktopServices
        function onScreenshotVisibleChanged() { window.visible = !Controller.previewMode && DesktopServices.screenshotVisible }
    }
    ScreenshotToolbar { anchors.centerIn: parent; width: implicitWidth; height: implicitHeight; scale: window.popupScale; activeShortcuts: window.visible }
}
