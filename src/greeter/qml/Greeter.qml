// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: root
    visible: true
    visibility: Window.FullScreen
    color: "#08110E"
    title: qsTr("Project Fidan Login")
    property bool signInVisible: false
    property date currentTime: new Date()
    property url wallpaperSource: "file:///usr/share/backgrounds/fidan/fidan-mountain-dawn-4k.png"
    Timer { interval: 1000; running: true; repeat: true; onTriggered: root.currentTime = new Date() }
    Shortcut { sequence: "Return"; enabled: !root.signInVisible; onActivated: { root.signInVisible = true; username.forceActiveFocus() } }

    Image { anchors.fill: parent; source: root.wallpaperSource; fillMode: Image.PreserveAspectCrop }
    Rectangle { anchors.fill: parent; color: "#48040B08" }

    Column {
        anchors.centerIn: parent
        width: 360
        spacing: 15
        Text { anchors.horizontalCenter: parent.horizontalCenter; text: Qt.formatTime(root.currentTime, "HH:mm"); color: "white"; font.pixelSize: 54; font.weight: Font.Light }
        Text { anchors.horizontalCenter: parent.horizontalCenter; text: Qt.formatDate(root.currentTime, "dddd, d MMMM"); color: "#CFD9D4"; font.pixelSize: 14 }
        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter; width: 78; height: 78; radius: 39
            color: "#DDE9E3"
            Item {
                anchors.centerIn: parent
                width: 48
                height: 48
                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 17
                    height: 17
                    radius: 9
                    color: "#56685F"
                }
                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottom: parent.bottom
                    width: 38
                    height: 23
                    radius: 12
                    color: "#56685F"
                }
            }
        }
        Button { visible: !root.signInVisible; width: parent.width; text: qsTr("Sign in"); onClicked: { root.signInVisible = true; username.forceActiveFocus() } }
        TextField { id: username; objectName: "greeterUsername"; visible: root.signInVisible; onAccepted: GreeterBackend.beginLogin(text); width: parent.width; placeholderText: qsTr("Username"); color: "white"; enabled: !password.visible && !GreeterBackend.busy }
        TextField {
            id: password; objectName: "greeterPassword"; width: parent.width; placeholderText: GreeterBackend.prompt.length ? GreeterBackend.prompt : qsTr("Password")
            color: "white"; echoMode: GreeterBackend.secretPrompt ? TextInput.Password : TextInput.Normal; visible: GreeterBackend.prompt.length > 0; enabled: !GreeterBackend.busy
            onAccepted: { GreeterBackend.answer(text); text = "" }
            onVisibleChanged: if (visible) forceActiveFocus()
            onEnabledChanged: if (enabled && visible) forceActiveFocus()
        }
        Button {
            width: parent.width
            visible: root.signInVisible
            enabled: !GreeterBackend.busy
            text: password.visible ? qsTr("Unlock") : qsTr("Continue")
            onClicked: { if (password.visible) { GreeterBackend.answer(password.text); password.text = "" } else GreeterBackend.beginLogin(username.text) }
        }
        Text { width: parent.width; text: GreeterBackend.error; color: "#FF9898"; wrapMode: Text.Wrap; horizontalAlignment: Text.AlignHCenter }
    }
    Row {
        anchors.left: parent.left; anchors.bottom: parent.bottom; anchors.margins: 22; spacing: 12
        Button { text: qsTr("Shut down"); onClicked: GreeterBackend.powerOff() }
        Button { text: qsTr("Restart"); onClicked: GreeterBackend.reboot() }
    }
}

