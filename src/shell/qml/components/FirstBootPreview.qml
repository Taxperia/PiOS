// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Fidan.Shell

Item {
    id: root
    property int step: 0
    property string validationError: ""

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0; color: "#173027" }
            GradientStop { position: 1; color: "#07100D" }
        }
    }

    Row {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top; anchors.topMargin: 32
        spacing: 8
        Repeater {
            model: 6
            Rectangle {
                required property int index
                width: index === root.step ? 32 : 9; height: 9; radius: 5
                color: index <= root.step ? Theme.accent : "#41534B"
                Behavior on width { NumberAnimation { duration: Theme.motionNormal } }
            }
        }
    }

    Button {
        anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 24
        text: qsTr("Skip preview")
        onClicked: Controller.completeFirstBootPreview()
        background: Rectangle { radius: 8; color: "#253A312D"; border.color: "#53675D" }
        contentItem: Text { text: parent.text; color: Theme.muted(true); font.family: Theme.fontFamily; font.pixelSize: 10; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: 24
        anchors.topMargin: 24
        width: 290
        height: 34
        radius: 8
        color: "#D925352F"
        border.width: 1
        border.color: "#52705F"
        Text {
            anchors.centerIn: parent
            text: qsTr("VISUAL PREVIEW — no account is created")
            color: "#D8E8DF"
            font.family: Theme.fontFamily
            font.pixelSize: 10
            font.weight: Font.DemiBold
        }
    }

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 100, 700)
        spacing: 22

        SystemIcon {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 58; Layout.preferredHeight: 58
            name: root.step === 3 ? "wifi" : root.step === 5 ? "home" : "menu"
            color: Theme.accent
        }
        Text {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            text: root.step === 0 ? qsTr("Welcome to Project Fidan")
                 : root.step === 1 ? qsTr("Language and region")
                 : root.step === 2 ? qsTr("Create your account")
                 : root.step === 3 ? qsTr("Connect to Wi-Fi")
                 : root.step === 4 ? qsTr("Privacy and remote access") : qsTr("You're ready")
            color: Theme.text(true); font.family: Theme.fontFamily; font.pixelSize: 30; font.weight: Font.Bold
        }
        Text {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            text: root.step === 0 ? qsTr("A lightweight Debian desktop built for Raspberry Pi 5.")
                 : root.step === 5 ? qsTr("Your desktop is ready to use.") : ""
            color: Theme.muted(true); font.family: Theme.fontFamily; font.pixelSize: 14
        }

        StackLayout {
            currentIndex: root.step
            Layout.fillWidth: true
            Layout.preferredHeight: 240

            Item {
                Column { anchors.centerIn: parent; spacing: 10
                    Text { anchors.horizontalCenter: parent.horizontalCenter; text: qsTr("Fast, private and designed for makers"); color: Theme.text(true); font.family: Theme.fontFamily; font.pixelSize: 16 }
                    Text { anchors.horizontalCenter: parent.horizontalCenter; text: qsTr("This wizard configures the device before the first desktop session."); color: Theme.muted(true); font.family: Theme.fontFamily; font.pixelSize: 11 }
                }
            }
            ColumnLayout {
                spacing: 10
                ComboBox { id: locale; Layout.fillWidth: true; model: ["Türkçe (Türkiye)", "English (United States)"] }
                ComboBox { id: keyboard; Layout.fillWidth: true; model: ["Türkçe Q", "English US"] }
                ComboBox { id: timezone; Layout.fillWidth: true; model: ["Europe/Istanbul", "Europe/London", "UTC"] }
            }
            ColumnLayout {
                spacing: 10
                TextField { id: displayName; Layout.fillWidth: true; placeholderText: qsTr("Your name") }
                TextField { id: username; Layout.fillWidth: true; placeholderText: qsTr("Username") }
                TextField { id: accountPassword; Layout.fillWidth: true; placeholderText: qsTr("Password (at least 8 characters)"); echoMode: TextInput.Password }
            }
            ColumnLayout {
                spacing: 10
                TextField { id: ssid; Layout.fillWidth: true; placeholderText: qsTr("Wi-Fi network name") }
                TextField { id: wifiPassword; Layout.fillWidth: true; placeholderText: qsTr("Wi-Fi password"); echoMode: TextInput.Password }
                CheckBox { id: connectLater; text: qsTr("Connect later") }
            }
            ColumnLayout {
                spacing: 12
                CheckBox { id: enableSsh; text: qsTr("Enable remote access with SSH"); checked: false }
                CheckBox { id: diagnostics; text: qsTr("Send anonymous diagnostic information"); checked: false }
                Text { Layout.fillWidth: true; text: qsTr("Both options start disabled. SSH is useful when the Raspberry Pi has no monitor."); color: Theme.muted(true); font.family: Theme.fontFamily; font.pixelSize: 11; wrapMode: Text.Wrap }
            }
            Item {
                Column { anchors.centerIn: parent; spacing: 12
                    Rectangle { anchors.horizontalCenter: parent.horizontalCenter; width: 72; height: 72; radius: 36; color: "#28583F"; SystemIcon { anchors.centerIn: parent; width: 38; height: 38; name: "home"; color: Theme.accent } }
                    Text { anchors.horizontalCenter: parent.horizontalCenter; text: qsTr("Project Fidan is configured"); color: Theme.text(true); font.family: Theme.fontFamily; font.pixelSize: 15; font.weight: Font.DemiBold }
                }
            }
        }

        Text { Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter; text: root.validationError; color: "#FF9D9D"; font.family: Theme.fontFamily; font.pixelSize: 10 }

        RowLayout {
            Layout.fillWidth: true
            Button {
                visible: root.step > 0 && root.step < 5
                text: qsTr("Back")
                onClicked: { root.validationError = ""; root.step-- }
            }
            Item { Layout.fillWidth: true }
            Button {
                text: root.step === 5 ? qsTr("Open desktop") : qsTr("Continue")
                onClicked: root.advance()
                background: Rectangle { radius: 10; color: Theme.accent }
                contentItem: Text { text: parent.text; color: "#07110B"; font.family: Theme.fontFamily; font.pixelSize: 12; font.weight: Font.Bold; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
        }
    }

    function advance() {
        validationError = ""
        if (step === 2 && (username.text.trim().length < 3 || accountPassword.text.length < 8)) {
            validationError = qsTr("Enter a username and a password of at least 8 characters.")
            return
        }
        if (step === 3 && !connectLater.checked && ssid.text.trim().length === 0) {
            validationError = qsTr("Enter a Wi-Fi network or choose Connect later.")
            return
        }
        if (step === 5) {
            accountPassword.text = ""
            wifiPassword.text = ""
            Controller.completeFirstBootPreview()
            return
        }
        step++
    }
}
