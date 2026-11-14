// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    objectName: "firstBootWindow"
    width: Math.min(1020, Screen.desktopAvailableWidth - 64)
    height: Math.min(650, Screen.desktopAvailableHeight - 96)
    minimumWidth: 420; minimumHeight: 350
    visible: true; visibility: Window.Windowed
    flags: Qt.Dialog | Qt.FramelessWindowHint
    color: "transparent"; title: qsTr("Welcome to PiOS")
    palette.window: "#1C2133"; palette.base: "#252C41"; palette.text: "#F1F3FA"
    palette.windowText: "#F1F3FA"; palette.button: "#303950"; palette.buttonText: "#F1F3FA"
    palette.highlight: "#A8B8FF"; palette.highlightedText: "#17213E"
    palette.placeholderText: "#A5ADC3"
    readonly property bool wide: width >= 820
    property int step: 0
    property bool restored: false
    readonly property var titles: [qsTr("Welcome to PiOS"), qsTr("Language and region"), qsTr("Create your account"), qsTr("Connect to Wi-Fi"), qsTr("Privacy"), qsTr("System updates"), qsTr("Ready"), qsTr("Setup complete")]
    readonly property bool accountValid: /^[a-z][a-z0-9_-]{1,30}$/.test(username.text) && displayName.text.length <= 80
        && ((Setup.state.accountReady && password.text.length === 0 && displayName.text === Setup.state.displayName)
        || (password.text.length >= 8 && password.text === confirmation.text))
    Component.onCompleted: Setup.refresh()
    Connections {
        target: Setup
        function onStateChanged() {
            const state = Setup.state
            if (!root.restored && !state.error && Object.keys(state).length) {
                root.restored = true
                if (state.locale) { locale.currentIndex = state.locale.startsWith("tr") ? 0 : 1; Setup.setLanguage(state.locale) }
                if (state.keymap) keymap.currentIndex = state.keymap === "trq" ? 0 : 1
                if (state.timezone) timezone.currentIndex = Math.max(0, timezone.model.indexOf(state.timezone))
                if (state.username) username.text = state.username
                if (state.displayName) displayName.text = state.displayName
                if (state.ssid) ssid.text = state.ssid
                enableSsh.checked = !!state.sshEnabled
                enableRemote.checked = !!state.remoteEnabled
                root.step = state.complete ? 7 : state.username || state.localeReady ? state.step : 0
            }
        }
        function onOperationFinished(operation, success) {
            if (!success) return
            if (operation === "SetLocale") root.step = 2
            else if (operation === "CreateUser") { password.clear(); confirmation.clear(); root.step = 3; Setup.scanWifi() }
            else if (operation === "ConnectWifi" || operation === "SkipNetwork") { wifiPassword.clear(); root.step = 4 }
            else if (operation === "SetPrivacy") root.step = 5
            else if (operation === "SkipUpdates") root.step = 6
            else if (operation === "Complete") root.step = 7
        }
    }
    Shortcut { sequence: "Alt+Left"; enabled: !Setup.busy && root.step > 0 && root.step < 7; onActivated: root.step-- }
    Rectangle { anchors.fill: parent; radius: 22; color: "#1C2133"; border.color: "#41485E" }
    Rectangle {
        visible: root.wide; anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom
        anchors.margins: 1; width: 235; radius: 21; color: "#252D46"
        ColumnLayout {
            anchors.fill: parent; anchors.margins: 26; spacing: 16
            Label { text: "PiOS"; font.pixelSize: 27; font.bold: true; Layout.bottomMargin: 20 }
            Repeater {
                model: root.titles.slice(1, 7)
                RowLayout {
                    required property int index
                    required property string modelData
                    Layout.fillWidth: true; spacing: 12
                    Rectangle {
                        width: 25; height: 25; radius: 13
                        color: root.step === index + 1 ? "#A8B8FF" : "transparent"
                        border.color: root.step === index + 1 ? "#A8B8FF" : "#67728E"
                        Label { anchors.centerIn: parent; text: root.step > index + 1 ? "✓" : String(index + 1); color: root.step === index + 1 ? "#17213E" : "#A5ADC3"; font.pixelSize: 11 }
                    }
                    Label { Layout.fillWidth: true; text: modelData; wrapMode: Text.WordWrap; font.pixelSize: 12; color: root.step === index + 1 ? "#F1F3FA" : "#A5ADC3" }
                }
            }
            Item { Layout.fillHeight: true }
            Label { Layout.fillWidth: true; text: qsTr("Your progress is saved automatically."); wrapMode: Text.WordWrap; color: "#A5ADC3"; font.pixelSize: 11 }
        }
    }
    ColumnLayout {
        anchors.fill: parent; anchors.margins: Math.min(32, parent.width / 20); anchors.leftMargin: root.wide ? 267 : Math.min(32, parent.width / 20); spacing: 16
        RowLayout {
            Layout.fillWidth: true
            Label { visible: !root.wide; text: "PiOS"; font.pixelSize: 23; font.bold: true; color: "#F1F3FA" }
            Item { Layout.fillWidth: true }
            Label { visible: root.step > 0 && root.step < 7; text: qsTr("Step %1 of 6").arg(Math.min(root.step, 6)); color: "#A5ADC3" }
        }
        ProgressBar {
            id: progress; Layout.fillWidth: true; from: 0; to: 6; value: Math.min(root.step, 6); visible: root.step < 7
            background: Rectangle { implicitHeight: 3; color: "#303950"; radius: 2 }
            contentItem: Item { implicitHeight: 3; Rectangle { width: progress.visualPosition * parent.width; height: parent.height; radius: 2; color: "#A8B8FF" } }
        }
        ScrollView {
            Layout.fillWidth: true; Layout.fillHeight: true; clip: true; contentWidth: availableWidth
            ColumnLayout {
                width: Math.min(720, parent.width); anchors.horizontalCenter: parent.horizontalCenter; spacing: 20
                Label { Layout.fillWidth: true; text: root.titles[root.step]; color: "#F1F3FA"; font.pixelSize: 30; font.bold: true; wrapMode: Text.WordWrap }
                Label { Layout.fillWidth: true; color: "#A5ADC3"; wrapMode: Text.WordWrap; text: root.step === 0 ? qsTr("Set up your language, account and network. Your progress is saved automatically; passwords are never saved in the setup history.") : root.step === 6 ? qsTr("Your account is ready. Finish to enable the login screen and restart.") : root.step === 7 ? qsTr("Setup is complete. The device will restart. If it does not, restart it manually to sign in.") : ""; visible: text.length > 0 }
                StackLayout {
                    currentIndex: root.step; Layout.fillWidth: true
                    Item { implicitHeight: 90 }
                    ColumnLayout {
                        spacing: 12
                        Label { text: qsTr("Language") }
                        ComboBox { id: locale; objectName: "setupLanguage"; Layout.fillWidth: true; model: ["Türkçe", "English"]; onActivated: Setup.setLanguage(currentIndex === 0 ? "tr_TR.UTF-8" : "en_US.UTF-8") }
                        Label { text: qsTr("Keyboard layout") }
                        ComboBox { id: keymap; Layout.fillWidth: true; model: [qsTr("Turkish Q"), qsTr("English US")] }
                        Label { text: qsTr("Time zone") }
                        ComboBox { id: timezone; Layout.fillWidth: true; model: ["Europe/Istanbul", "Europe/London", "UTC"] }
                        TextField { Layout.fillWidth: true; placeholderText: qsTr("Test your keyboard here"); Accessible.name: placeholderText }
                    }
                    ColumnLayout {
                        spacing: 12
                        TextField { id: displayName; objectName: "setupDisplayName"; Layout.fillWidth: true; placeholderText: qsTr("Your name"); maximumLength: 80; Accessible.name: placeholderText }
                        TextField { id: username; objectName: "setupUsername"; Layout.fillWidth: true; placeholderText: qsTr("Username (lowercase letters and numbers)"); readOnly: !!Setup.state.username; maximumLength: 31; Accessible.name: placeholderText }
                        TextField { id: password; objectName: "setupPassword"; Layout.fillWidth: true; placeholderText: Setup.state.accountReady ? qsTr("New password (leave blank to keep it)") : qsTr("Password (at least 8 characters)"); echoMode: reveal.checked ? TextInput.Normal : TextInput.Password; maximumLength: 256; Accessible.name: placeholderText }
                        TextField { id: confirmation; objectName: "setupPasswordConfirmation"; Layout.fillWidth: true; placeholderText: qsTr("Repeat password"); echoMode: reveal.checked ? TextInput.Normal : TextInput.Password; maximumLength: 256; Accessible.name: placeholderText }
                        CheckBox { id: reveal; text: qsTr("Show passwords") }
                        Label { Layout.fillWidth: true; text: qsTr("Use 2–31 lowercase letters, digits, underscores or hyphens for your username. Start with a letter."); wrapMode: Text.WordWrap; color: "#A5ADC3" }
                        Label { visible: confirmation.text.length > 0 && confirmation.text !== password.text; text: qsTr("The passwords do not match."); color: "#FFB6AE" }
                    }
                    ColumnLayout {
                        spacing: 12
                        RowLayout {
                            Layout.fillWidth: true
                            ComboBox { objectName: "setupWifiNetworks"; Layout.fillWidth: true; model: Setup.networks; displayText: currentIndex < 0 ? qsTr("Choose a network") : currentText; onActivated: ssid.text = currentText }
                            Button { text: qsTr("Scan again"); enabled: !Setup.busy; onClicked: Setup.scanWifi() }
                        }
                        TextField { id: ssid; Layout.fillWidth: true; placeholderText: qsTr("Network name (including hidden networks)"); maximumLength: 32; Accessible.name: placeholderText }
                        TextField { id: wifiPassword; Layout.fillWidth: true; placeholderText: qsTr("Wi-Fi password"); echoMode: TextInput.Password; maximumLength: 63; Accessible.name: placeholderText }
                        Label { Layout.fillWidth: true; text: qsTr("Ethernet works without this step. You can also continue offline and connect later."); color: "#A5ADC3"; wrapMode: Text.WordWrap }
                        Button { objectName: "setupSkipNetwork"; text: qsTr("Continue without Wi-Fi"); enabled: !Setup.busy; onClicked: Setup.skipNetwork() }
                    }
                    ColumnLayout {
                        spacing: 12
                        CheckBox { id: enableSsh; text: qsTr("Enable remote access with SSH"); checked: false }
                        CheckBox { id: enableRemote; text: qsTr("Enable the browser desktop on my local network"); checked: false }
                        Label { Layout.fillWidth: true; text: qsTr("SSH starts disabled. Anonymous diagnostic collection is disabled. You can change remote access later in Settings."); color: "#A5ADC3"; wrapMode: Text.WordWrap }
                    }
                    ColumnLayout {
                        spacing: 12
                        Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; text: qsTr("Download the latest signed PiOS packages now, or finish setup offline. An update does not change your personal files."); color: "#A5ADC3" }
                        Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; text: Setup.state.updateBusy ? qsTr("Updating… Keep the device powered on.") : Setup.state.updateDone ? qsTr("You can continue.") : Setup.state.update && Setup.state.update.updateAvailable ? qsTr("A PiOS update is available.") : qsTr("Check for available updates.") }
                        Label { Layout.fillWidth: true; visible: !!Setup.state.updateError; text: Setup.state.updateError || ""; wrapMode: Text.WordWrap; color: "#FFB6AE" }
                        Button {
                            objectName: "setupUpdate"
                            text: Setup.state.update && Setup.state.update.downloaded ? qsTr("Install update") : Setup.state.update && Setup.state.update.updateAvailable && Setup.state.update.installable ? qsTr("Download update") : qsTr("Check for updates")
                            enabled: !Setup.busy
                            onClicked: Setup.startUpdate(Setup.state.update && Setup.state.update.downloaded ? "install" : Setup.state.update && Setup.state.update.updateAvailable && Setup.state.update.installable ? "download" : "check")
                        }
                        Button { objectName: "setupSkipUpdate"; text: qsTr("Continue without updating"); enabled: !Setup.busy; onClicked: Setup.skipUpdates() }
                    }
                    Item { implicitHeight: 80 }
                    Item { implicitHeight: 80 }
                }
                Label { Layout.fillWidth: true; text: Setup.error; visible: text.length > 0; color: "#FFB6AE"; wrapMode: Text.WordWrap }
            }
        }
        RowLayout {
            Layout.fillWidth: true; spacing: 12
            Button { objectName: "setupBack"; text: qsTr("Back"); visible: root.step > 0 && root.step < 7; enabled: !Setup.busy; onClicked: root.step-- }
            BusyIndicator { running: Setup.busy; visible: running; Layout.preferredWidth: 30; Layout.preferredHeight: 30 }
            Item { Layout.fillWidth: true }
            Button {
                objectName: "setupContinue"
                palette.button: "#A8B8FF"; palette.buttonText: "#17213E"; font.bold: true
                text: root.step === 6 ? qsTr("Finish and restart") : root.step === 7 ? qsTr("Close") : qsTr("Continue")
                enabled: !Setup.busy && (root.step !== 2 || root.accountValid) && (root.step !== 5 || !!Setup.state.updateDone)
                onClicked: {
                    if (root.step === 0) root.step = 1
                    else if (root.step === 1) Setup.setLocale(locale.currentIndex === 0 ? "tr_TR.UTF-8" : "en_US.UTF-8", keymap.currentIndex === 0 ? "trq" : "us", timezone.currentText)
                    else if (root.step === 2) {
                        if (Setup.state.accountReady && password.text.length === 0) { root.step = 3; Setup.scanWifi() }
                        else Setup.createUser(username.text, displayName.text, password.text)
                    } else if (root.step === 3) { if (ssid.text.length) Setup.connectWifi(ssid.text, wifiPassword.text); else Setup.skipNetwork() }
                    else if (root.step === 4) Setup.setPrivacy(enableSsh.checked, false, enableRemote.checked)
                    else if (root.step === 5) root.step = 6
                    else if (root.step === 6) Setup.complete()
                    else Qt.quit()
                }
            }
        }
    }
}
