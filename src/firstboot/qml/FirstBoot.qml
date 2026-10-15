// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    width: 1100
    height: 680
    visible: true
    visibility: Window.FullScreen
    color: "#0A1411"
    title: qsTr("Welcome to Project Fidan")
    property int step: 0

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0; color: "#152720" }
            GradientStop { position: 1; color: "#08100D" }
        }
    }

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 80, 720)
        spacing: 24
        Label {
            Layout.fillWidth: true
            text: step === 0 ? qsTr("Welcome to Project Fidan")
                 : step === 1 ? qsTr("Language and region")
                 : step === 2 ? qsTr("Create your account")
                 : step === 3 ? qsTr("Connect to Wi‑Fi")
                 : step === 4 ? qsTr("Privacy") : qsTr("Ready")
            color: "#F5F8F6"
            font.pixelSize: 32
            font.bold: true
        }
        Label {
            Layout.fillWidth: true
            text: step === 0 ? qsTr("A lightweight Debian desktop built for Raspberry Pi 5.")
                 : step === 5 ? qsTr("Setup is complete. The device will restart now.") : ""
            color: "#AEBBB5"
            font.pixelSize: 16
            wrapMode: Text.Wrap
        }
        StackLayout {
            currentIndex: step
            Layout.fillWidth: true
            Layout.preferredHeight: 270
            Item {}
            ColumnLayout {
                ComboBox { id: locale; Layout.fillWidth: true; model: ["tr_TR.UTF-8", "en_US.UTF-8"] }
                ComboBox { id: keymap; Layout.fillWidth: true; model: ["trq", "us"] }
                ComboBox { id: timezone; Layout.fillWidth: true; model: ["Europe/Istanbul", "Europe/London", "UTC"] }
            }
            ColumnLayout {
                TextField { id: displayName; Layout.fillWidth: true; placeholderText: qsTr("Your name") }
                TextField { id: username; Layout.fillWidth: true; placeholderText: qsTr("Username") }
                TextField { id: password; Layout.fillWidth: true; placeholderText: qsTr("Password (at least 8 characters)"); echoMode: TextInput.Password }
            }
            ColumnLayout {
                TextField { id: ssid; Layout.fillWidth: true; placeholderText: qsTr("Wi‑Fi network name") }
                TextField { id: wifiPassword; Layout.fillWidth: true; placeholderText: qsTr("Wi‑Fi password"); echoMode: TextInput.Password }
                Label { text: qsTr("You can leave this blank and connect later."); color: "#AEBBB5" }
            }
            ColumnLayout {
                CheckBox { id: enableSsh; text: qsTr("Enable remote access with SSH"); checked: false }
                CheckBox { id: enableDiagnostics; text: qsTr("Send anonymous diagnostic information"); checked: false }
                Label {
                    Layout.fillWidth: true
                    text: qsTr("Both options start disabled and can be changed later.")
                    color: "#AEBBB5"
                    wrapMode: Text.Wrap
                }
            }
            Item {}
        }
        Label { Layout.fillWidth: true; text: Setup.error; color: "#FF8F8F"; visible: text.length > 0; wrapMode: Text.Wrap }
        RowLayout {
            Layout.fillWidth: true
            Item { Layout.fillWidth: true }
            Button {
                text: step === 5 ? qsTr("Finish") : qsTr("Continue")
                enabled: !Setup.busy
                onClicked: {
                    if (step === 0) { step = 1; return }
                    if (step === 1 && Setup.setLocale(locale.currentText, keymap.currentText, timezone.currentText)) { step = 2; return }
                    if (step === 2 && Setup.createUser(username.text, displayName.text, password.text)) { password.text = ""; step = 3; return }
                    if (step === 3 && (ssid.text.length === 0 || Setup.connectWifi(ssid.text, wifiPassword.text))) { wifiPassword.text = ""; step = 4; return }
                    if (step === 4 && Setup.setPrivacy(enableSsh.checked, enableDiagnostics.checked)) { step = 5; return }
                    if (step === 5 && Setup.complete()) Qt.quit()
                }
                background: Rectangle { radius: 10; color: parent.enabled ? "#46C878" : "#476052" }
                contentItem: Text { text: parent.text; color: "#07110B"; font.pixelSize: 14; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
        }
    }
}

