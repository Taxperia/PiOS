// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Control {
    id: root
    objectName: "welcomeWizard"
    background: Rectangle { radius: 22; color: Controller.darkTheme ? Controller.themeColor("#1C2133", Controller.themeMode, Controller.darkTheme) : Controller.themeColor("#F7F8FC", Controller.themeMode, Controller.darkTheme); border.color: Controller.darkTheme ? Controller.themeColor("#41485E", Controller.themeMode, Controller.darkTheme) : Controller.themeColor("#D9DEEA", Controller.themeMode, Controller.darkTheme) }
    readonly property color ink: Controller.darkTheme ? Controller.themeColor("#F1F3FA", Controller.themeMode, Controller.darkTheme) : Controller.themeColor("#202638", Controller.themeMode, Controller.darkTheme)
    readonly property color muted: Controller.darkTheme ? Controller.themeColor("#A5ADC3", Controller.themeMode, Controller.darkTheme) : Controller.themeColor("#626C83", Controller.themeMode, Controller.darkTheme)
    readonly property color setupAccent: Controller.themeColor("#A8B8FF", Controller.themeMode, Controller.darkTheme)
    readonly property bool wide: root.width >= 780
    palette.window: Controller.darkTheme ? Controller.themeColor("#1C2133", Controller.themeMode, Controller.darkTheme) : Controller.themeColor("#F7F8FC", Controller.themeMode, Controller.darkTheme)
    palette.windowText: root.ink
    palette.text: root.ink
    palette.buttonText: root.ink
    palette.button: Controller.darkTheme ? Controller.themeColor("#2B3249", Controller.themeMode, Controller.darkTheme) : Controller.themeColor("#E8ECF6", Controller.themeMode, Controller.darkTheme)
    palette.base: Controller.darkTheme ? Controller.themeColor("#242A3F", Controller.themeMode, Controller.darkTheme) : "white"
    palette.highlight: Controller.darkTheme ? Controller.themeColor("#A8B8FF", Controller.themeMode, Controller.darkTheme) : Controller.themeColor("#5268BD", Controller.themeMode, Controller.darkTheme)
    readonly property var titles: [qsTr("Welcome to PiOS"), qsTr("Language and region"), qsTr("Make it yours"), qsTr("Privacy and access"), qsTr("Stay up to date"), qsTr("Your desktop is ready")]
    readonly property int step: Controller.welcomeStep
    readonly property var accents: ["#63de8a", "#2589ff", "#ac55ef", "#ed449a", "#ff9546", "#ffd35d", "#ff575e", "#24c5cc"]
    property string saveError: ""
    function openSettings(page) { Controller.dismissWelcome(); Controller.openSettingsPage(page) }
    Rectangle {
        visible: root.wide; anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom; anchors.margins: 1
        width: 235; radius: 21; color: Controller.darkTheme ? Controller.themeColor("#252D46", Controller.themeMode, Controller.darkTheme) : Controller.themeColor("#EDF0F9", Controller.themeMode, Controller.darkTheme)
        ColumnLayout {
            anchors.fill: parent; anchors.margins: 26; spacing: 12
            Label { text: "PiOS"; color: root.ink; font.pixelSize: 27; font.bold: true; Layout.bottomMargin: 20 }
            Repeater {
                model: root.titles
                RowLayout {
                    required property int index
                    required property string modelData
                    Layout.fillWidth: true; spacing: 12
                    Rectangle {
                        width: 25; height: 25; radius: 13
                        color: index === root.step ? root.setupAccent : "transparent"
                        border.color: index === root.step ? root.setupAccent : root.muted
                        Label { anchors.centerIn: parent; text: index < root.step ? "✓" : String(index + 1); color: index === root.step ? Controller.themeColor("#1D2947", Controller.themeMode, Controller.darkTheme) : root.muted; font.pixelSize: 11 }
                    }
                    Label { Layout.fillWidth: true; text: modelData; wrapMode: Text.WordWrap; font.pixelSize: 12; color: index === root.step ? root.ink : root.muted }
                }
            }
            Item { Layout.fillHeight: true }
            Label { Layout.fillWidth: true; text: qsTr("Choose how your desktop looks and works. Changes are saved as you go, and you can change them later in Settings."); wrapMode: Text.WordWrap; font.pixelSize: 11; color: root.muted }
        }
    }
    ColumnLayout {
        anchors.fill: parent; anchors.margins: Math.min(32, root.width / 18); anchors.leftMargin: root.wide ? 267 : Math.min(32, root.width / 18); spacing: 18
        RowLayout {
            Layout.fillWidth: true
            Label { visible: !root.wide; text: "PiOS"; color: root.ink; font.pixelSize: 25; font.bold: true }
            Item { Layout.fillWidth: true }
            Button { objectName: "welcomeLater"; text: qsTr("Continue later"); onClicked: Controller.dismissWelcome() }
            Label { text: qsTr("%1 / 6").arg(root.step + 1); color: root.muted }
        }
        ProgressBar { Layout.fillWidth: true; from: 0; to: 5; value: root.step }
        Label { Layout.fillWidth: true; text: root.titles[root.step]; color: root.ink; font.pixelSize: 28; font.bold: true; wrapMode: Text.WordWrap }
        ScrollView {
            Layout.fillWidth: true; Layout.fillHeight: true; clip: true; contentWidth: availableWidth
            StackLayout {
                width: parent.width; currentIndex: root.step
                ColumnLayout {
                    spacing: 22
                    Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; color: root.ink; font.pixelSize: 18; text: qsTr("Press Next to finish setting up your operating system.") }
                    Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; color: root.muted; text: qsTr("Choose how your desktop looks and works. Changes are saved as you go, and you can change them later in Settings.") }
                    Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; color: root.muted; text: qsTr("Your local account is already ready. These preferences belong to your account.") }
                }
                ColumnLayout {
                    spacing: 10
                    Label { text: qsTr("Language for the next login"); color: root.ink }
                    ComboBox { objectName: "welcomeLanguage"; Layout.fillWidth: true; model: ["Türkçe", "English"]; currentIndex: Controller.sessionLanguage.startsWith("tr") ? 0 : 1; onActivated: Controller.sessionLanguage = currentIndex === 0 ? "tr_TR.UTF-8" : "en_US.UTF-8" }
                    Label { text: qsTr("Keyboard layout for the next login"); color: root.ink }
                    ComboBox { objectName: "welcomeKeyboard"; Layout.fillWidth: true; model: ["Türkçe Q", "English US", "Deutsch", "English UK"]; currentIndex: ["tr", "us", "de", "gb"].indexOf(Controller.keyboardLayout); onActivated: Controller.keyboardLayout = ["tr", "us", "de", "gb"][currentIndex] }
                    Label { text: qsTr("Date format"); color: root.ink }
                    ComboBox { objectName: "welcomeDateFormat"; Layout.fillWidth: true; model: [qsTr("Follow system locale"), "2026-11-11", "11.11.2026"]; currentIndex: ["locale", "iso", "day-first"].indexOf(Controller.dateFormat); onActivated: Controller.dateFormat = ["locale", "iso", "day-first"][currentIndex] }
                    Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; text: qsTr("Time zone: %1").arg(SettingsData.info.timezone || "UTC"); color: root.muted }
                    RowLayout {
                        Layout.fillWidth: true
                        TextField { id: timezone; Layout.fillWidth: true; text: SettingsData.info.timezone || "UTC"; placeholderText: "Europe/Istanbul"; Accessible.name: qsTr("Time zone"); maximumLength: 100 }
                        Button { text: qsTr("Apply"); enabled: !SettingsData.actionBusy && !Controller.mockMode; onClicked: SettingsData.setTimezone(timezone.text) }
                    }
                    Label { Layout.fillWidth: true; text: SettingsData.actionError; color: Controller.themeColor("#D65C52", Controller.themeMode, Controller.darkTheme); wrapMode: Text.WordWrap; visible: text.length > 0 }
                    Button { text: qsTr("Date and time settings"); onClicked: root.openSettings("System/date") }
                }
                ColumnLayout {
                    spacing: 10
                    Label { text: qsTr("Theme"); color: root.ink }
                    RowLayout {
                        Repeater { model: [{id:"light",name:qsTr("Light")},{id:"dark",name:"Dark"},{id:"green",name:"Green"},{id:"auto",name:qsTr("Automatic")}]
                            Button { required property var modelData; text: modelData.name; checkable: true; checked: Controller.themeMode === modelData.id; onClicked: Controller.themeMode = modelData.id }
                        }
                    }
                    Label { text: qsTr("Accent color"); color: root.ink }
                    Flow { Layout.fillWidth: true; spacing: 8
                        Repeater { model: root.accents
                            Button { required property string modelData; objectName: "welcomeAccent"; width: 42; height: 42; Accessible.name: qsTr("Accent color %1").arg(modelData); onClicked: Controller.accentColor = modelData
                                background: Rectangle { color: parent.modelData; radius: 21; border.width: Controller.accentColor === parent.modelData ? 3 : 0; border.color: root.ink }
                            }
                        }
                    }
                    GridLayout {
                        Layout.fillWidth: true; columns: root.width > 600 ? 2 : 1; columnSpacing: 15
                        Label { text: qsTr("Dock position"); color: root.ink }
                        ComboBox { Layout.fillWidth: true; model: [qsTr("Left"), qsTr("Bottom"), qsTr("Right")]; currentIndex: ["left","bottom","right"].indexOf(Controller.dockPosition); onActivated: Controller.dockPosition = ["left","bottom","right"][currentIndex] }
                        Label { text: qsTr("Panel position"); color: root.ink }
                        ComboBox { Layout.fillWidth: true; model: [qsTr("Top"), qsTr("Bottom")]; currentIndex: Controller.panelPosition === "top" ? 0 : 1; onActivated: Controller.panelPosition = currentIndex === 0 ? "top" : "bottom" }
                        Label { text: qsTr("Panel design"); color: root.ink }
                        ComboBox { Layout.fillWidth: true; model: [qsTr("Detailed"), qsTr("Compact")]; currentIndex: Controller.panelStyle === "detailed" ? 0 : 1; onActivated: Controller.panelStyle = currentIndex === 0 ? "detailed" : "compact" }
                    }
                    CheckBox { text: qsTr("Reduce animations"); checked: Controller.reducedMotion; onToggled: Controller.reducedMotion = checked }
                    Button { text: qsTr("More appearance settings"); onClicked: root.openSettings("Personalization") }
                }
                ColumnLayout {
                    spacing: 12
                    CheckBox { objectName: "welcomeSsh"; text: qsTr("Enable SSH access"); checked: !!SettingsData.info.sshEnabled; enabled: !!SettingsData.info.sshAvailable && !SettingsData.actionBusy && !Controller.mockMode; onToggled: SettingsData.setSshEnabled(checked) }
                    CheckBox { objectName: "welcomeBrowser"; text: qsTr("Enable the browser desktop on my local network"); checked: !!SettingsData.info.remoteEnabled; enabled: !!SettingsData.info.remoteAvailable && !SettingsData.actionBusy && !Controller.mockMode; onToggled: SettingsData.setRemoteEnabled(checked) }
                    Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; color: root.muted; text: qsTr("Use your local account for SSH and https://%1.local. Changing access requires administrator authorization.").arg(SettingsData.info.host || "fidan") }
                    Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; color: root.muted; text: qsTr("Location depends on an available provider and application permissions. Review permissions before granting access; no location is requested by this welcome screen.") }
                    Button { text: qsTr("Review location and application permissions"); onClicked: root.openSettings("Privacy/Data") }
                    CheckBox { text: qsTr("Share diagnostic reports (not available yet)"); checked: false; enabled: false }
                    Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; color: root.muted; text: qsTr("PiOS has no diagnostic upload service. No consent is recorded for a future service.") }
                    Label { Layout.fillWidth: true; visible: SettingsData.actionError.length > 0; text: SettingsData.actionError; wrapMode: Text.WordWrap; color: Controller.themeColor("#D65C52", Controller.themeMode, Controller.darkTheme) }
                }
                ColumnLayout {
                    spacing: 15
                    Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; color: root.muted; text: qsTr("PiOS checks GitHub Releases every six hours. Only packages verified with the pinned signing key and checksums can be installed. Updates never force a restart.") }
                    CheckBox { text: qsTr("Install signed PiOS updates automatically"); checked: Updates.automaticEnabled; enabled: !Updates.busy && !Controller.mockMode; onToggled: Updates.setAutomaticEnabled(checked) }
                    Button { text: qsTr("Check for updates"); enabled: !Updates.busy && !Controller.mockMode; onClicked: Updates.check() }
                    Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; color: root.muted; text: Updates.statusText }
                    Button { text: qsTr("Open updates and recovery"); onClicked: root.openSettings("System") }
                }
                ColumnLayout {
                    spacing: 18
                    Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; color: root.ink; text: qsTr("All done. You can reopen this welcome screen from Settings.") }
                    Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; color: root.muted; text: qsTr("Theme and panel changes are already applied. Language and keyboard changes take effect when you sign in again.") }
                }
            }
        }
        Label { visible: root.saveError.length > 0; text: root.saveError; color: Controller.themeColor("#D65C52", Controller.themeMode, Controller.darkTheme); Layout.fillWidth: true; wrapMode: Text.WordWrap }
        RowLayout {
            Layout.fillWidth: true
            Button { objectName: "welcomeBack"; text: qsTr("Back"); visible: root.step > 0; onClicked: Controller.welcomeStep = root.step - 1 }
            Item { Layout.fillWidth: true }
            Button { objectName: "welcomeNext"; text: root.step === 5 ? qsTr("Open desktop") : qsTr("Next"); enabled: !SettingsData.actionBusy && !Updates.busy
                palette.button: root.setupAccent; palette.buttonText: Controller.themeColor("#1D2947", Controller.themeMode, Controller.darkTheme); font.bold: true
                onClicked: { if (root.step < 5) Controller.welcomeStep = root.step + 1; else if (!Controller.finishWelcome()) root.saveError = qsTr("Preferences could not be saved. Check free disk space and retry.") }
            }
        }
    }
}
