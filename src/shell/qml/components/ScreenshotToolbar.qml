// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Fidan.Shell

GlassPanel {
    id: root
    implicitWidth: 620
    implicitHeight: 170
    raised: true
    property string selectedMode: "region"
    property bool activeShortcuts: false
    Shortcut { sequence: "Escape"; enabled: root.activeShortcuts; onActivated: DesktopServices.screenshotVisible = false }
    Shortcut { sequence: "Return"; enabled: root.activeShortcuts; onActivated: DesktopServices.captureScreenshot(root.selectedMode) }
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 16; spacing: 10
        RowLayout {
            Layout.fillWidth: true
            Text { Layout.fillWidth: true; text: qsTr("Screenshot"); color: Theme.text(Controller.darkTheme); font.family: Theme.fontFamily; font.pixelSize: 14; font.weight: Font.DemiBold }
            Text { text: qsTr("Escape to cancel"); color: Theme.muted(Controller.darkTheme); font.pixelSize: 10 }
            IconButton { objectName: "screenshotClose"; compact: true; label: "×"; accessibleName: qsTr("Close"); onClicked: DesktopServices.screenshotVisible = false }
        }
        RowLayout {
            Layout.fillWidth: true; spacing: 10
            Repeater {
                model: [{id:"region", title:qsTr("Region"), icon:"select"}, {id:"window", title:qsTr("Window"), icon:"window"}, {id:"screen", title:qsTr("Entire screen"), icon:"monitor"}]
                delegate: Rectangle {
                    required property var modelData
                    objectName: "screenshotMode-" + modelData.id
                    Layout.fillWidth: true; Layout.preferredHeight: 48
                    radius: 10; color: root.selectedMode === modelData.id ? Theme.accentStrong : Theme.surface(Controller.darkTheme)
                    border.width: 1; border.color: root.selectedMode === modelData.id ? Theme.accent : Theme.border(Controller.darkTheme)
                    Row { anchors.centerIn: parent; spacing: 8
                        SystemIcon { width: 18; height: 18; name: modelData.icon; color: root.selectedMode === modelData.id ? "white" : Theme.text(Controller.darkTheme) }
                        Text { text: modelData.title; color: root.selectedMode === modelData.id ? "white" : Theme.text(Controller.darkTheme); font.family: Theme.fontFamily; font.pixelSize: 11 }
                    }
                    Accessible.role: Accessible.RadioButton; Accessible.name: modelData.title
                    activeFocusOnTab: true
                    Keys.onSpacePressed: root.selectedMode = modelData.id
                    TapHandler { onTapped: root.selectedMode = modelData.id }
                }
            }
            Button { objectName: "screenshotCapture"; text: qsTr("Capture"); highlighted: true; palette.highlight: Theme.accentStrong; onClicked: DesktopServices.captureScreenshot(root.selectedMode) }
        }
        Text {
            Layout.fillWidth: true
            text: root.selectedMode === "window" ? qsTr("Drag around the window you want to capture.") : root.selectedMode === "region" ? qsTr("Drag to select an area after pressing Capture.") : qsTr("Capture all displays. Saved to Pictures/Screenshots and copied to the clipboard.")
            wrapMode: Text.Wrap; color: Theme.muted(Controller.darkTheme); font.family: Theme.fontFamily; font.pixelSize: 10
        }
    }
}
