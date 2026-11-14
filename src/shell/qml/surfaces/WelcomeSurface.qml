// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import Fidan.Shell

ApplicationWindow {
    id: window
    width: Math.min(920, Screen.width - 32)
    height: Math.min(700, Screen.height - 48)
    color: "transparent"
    title: qsTr("Welcome to PiOS")
    flags: Qt.FramelessWindowHint
    visible: false
    Component.onCompleted: {
        LayerShell.configure(window, "welcome")
        visible = Qt.binding(function() { return !Controller.previewMode && Controller.firstBootVisible })
    }
    Loader { anchors.fill: parent; active: window.visible; sourceComponent: Controller.setupSession ? deviceWelcome : personalWelcome }
    Component { id: deviceWelcome; DeviceWelcomeWizard {} }
    Component { id: personalWelcome; WelcomeWizard {} }
}
