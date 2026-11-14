// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Fidan.Shell

ColumnLayout {
    id: root
    spacing: 10
    Text { text: qsTr("Audio output"); color: Theme.muted(Controller.darkTheme); font.family: Theme.fontFamily; font.pixelSize: 11 }
    ComboBox {
        objectName: "audioOutputSelector"
        Layout.fillWidth: true
        model: DesktopServices.audioOutputs
        textRole: "name"; valueRole: "id"
        currentIndex: count > 0 ? DesktopServices.audioOutputs.findIndex(output => output.id === DesktopServices.defaultOutput) : -1
        enabled: count > 0
        Accessible.name: qsTr("Audio output")
        onActivated: index => DesktopServices.selectOutput(DesktopServices.audioOutputs[index].id)
        palette.button: Theme.raised(Controller.darkTheme)
        palette.buttonText: Theme.text(Controller.darkTheme)
        palette.text: Theme.text(Controller.darkTheme)
        palette.window: Theme.raised(Controller.darkTheme)
        palette.base: Theme.raised(Controller.darkTheme)
        palette.highlight: Theme.accentStrong
        font.family: Theme.fontFamily; font.pixelSize: 11
    }
    Text { text: qsTr("Application volume"); color: Theme.muted(Controller.darkTheme); font.family: Theme.fontFamily; font.pixelSize: 11 }
    Text {
        Layout.fillWidth: true; visible: DesktopServices.audioStreams.length === 0
        text: qsTr("Apps appear here while playing audio.")
        wrapMode: Text.Wrap; color: Theme.muted(Controller.darkTheme); font.family: Theme.fontFamily; font.pixelSize: 10
    }
    Repeater {
        model: DesktopServices.audioStreams
        delegate: ColumnLayout {
            id: streamRow
            required property var modelData
            Layout.fillWidth: true
            spacing: 0
            RowLayout {
                Layout.fillWidth: true
                Text { Layout.fillWidth: true; text: streamRow.modelData.name; elide: Text.ElideRight; color: Theme.text(Controller.darkTheme); font.family: Theme.fontFamily; font.pixelSize: 11 }
                Text { text: Math.round(appSlider.value) + "%"; color: Theme.muted(Controller.darkTheme); font.pixelSize: 10 }
                IconButton {
                    compact: true; iconName: streamRow.modelData.muted ? "volume-muted" : "volume"
                    accessibleName: qsTr("Toggle mute for %1").arg(streamRow.modelData.name)
                    onClicked: DesktopServices.toggleApplicationMute(streamRow.modelData.id)
                }
            }
            Slider {
                id: appSlider
                objectName: "applicationVolume-" + streamRow.modelData.id
                Layout.fillWidth: true
                from: 0; to: 100; stepSize: 1; value: streamRow.modelData.volume
                palette.highlight: Theme.accent; palette.button: Theme.accent
                Accessible.name: qsTr("Volume for %1").arg(streamRow.modelData.name)
                background: Rectangle {
                    x: appSlider.leftPadding; y: appSlider.topPadding + appSlider.availableHeight / 2 - height / 2
                    width: appSlider.availableWidth; height: 6; radius: 3
                    color: Controller.darkTheme ? Controller.themeColor("#394640", Controller.themeMode, Controller.darkTheme) : Controller.themeColor("#CDDAD2", Controller.themeMode, Controller.darkTheme)
                    Rectangle { width: parent.width * appSlider.visualPosition; height: parent.height; radius: 3; color: streamRow.modelData.muted ? Controller.themeColor("#718079", Controller.themeMode, Controller.darkTheme) : Theme.accent }
                }
                handle: Rectangle {
                    x: appSlider.leftPadding + appSlider.visualPosition * (appSlider.availableWidth - width)
                    y: appSlider.topPadding + appSlider.availableHeight / 2 - height / 2
                    width: 15; height: 15; radius: 8; color: Theme.accent
                    border.width: appSlider.activeFocus ? 2 : 1; border.color: Theme.text(Controller.darkTheme)
                }
                onMoved: writeDelay.restart()
                onPressedChanged: {
                    if (pressed) DesktopServices.setAudioEditing(true)
                    else {
                        writeDelay.stop()
                        DesktopServices.setApplicationVolume(streamRow.modelData.id, Math.round(value))
                        DesktopServices.setAudioEditing(false)
                    }
                }
                Component.onDestruction: if (pressed) DesktopServices.setAudioEditing(false)
                Timer { id: writeDelay; interval: 100; onTriggered: DesktopServices.setApplicationVolume(streamRow.modelData.id, Math.round(appSlider.value)) }
            }
        }
    }
}
