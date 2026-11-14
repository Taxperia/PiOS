// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Layouts
import Fidan.Shell

GlassPanel {
    id: root
    implicitWidth: 300
    implicitHeight: 88
    raised: true
    readonly property bool isBrightness: Controller.osdKind === "brightness"
    readonly property bool isMicrophone: Controller.osdKind === "microphone"
    readonly property bool silent: isMicrophone ? DesktopServices.microphoneMuted : !isBrightness && (Controller.volumeMuted || Controller.volume === 0)
    readonly property int level: isBrightness ? Controller.brightness : isMicrophone ? (DesktopServices.microphoneMuted ? 0 : 100) : Controller.volumeMuted ? 0 : Controller.volume
    readonly property string heading: isBrightness ? qsTr("Brightness") : isMicrophone ? qsTr("Microphone") : Controller.volumeMuted ? qsTr("Muted") : qsTr("Volume")
    readonly property string valueText: isMicrophone ? (silent ? qsTr("Off") : qsTr("On")) : !isBrightness && Controller.volumeMuted ? qsTr("Off") : level + "%"

    Accessible.role: Accessible.Indicator
    Accessible.name: heading + " " + valueText

    RowLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 14

        Rectangle {
            Layout.preferredWidth: 44
            Layout.preferredHeight: 44
            radius: 14
            color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, root.silent ? 0.08 : 0.16)
            SystemIcon {
                anchors.centerIn: parent
                width: 24; height: 24
                name: root.isBrightness ? "sun" : root.isMicrophone ? (root.silent ? "microphone-muted" : "microphone") : root.silent ? "volume-muted" : "volume"
                color: root.silent ? Theme.muted(Controller.darkTheme) : Theme.accent
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 10
            RowLayout {
                Layout.fillWidth: true
                Text {
                    Layout.fillWidth: true
                    text: root.heading
                    color: Theme.text(Controller.darkTheme)
                    font.family: Theme.fontFamily
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                }
                Text {
                    text: root.valueText
                    color: Theme.muted(Controller.darkTheme)
                    font.family: Theme.fontFamily
                    font.pixelSize: 12
                }
            }
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 6
                radius: 3
                color: Controller.darkTheme ? "#384B433E" : "#3054645C"
                Rectangle {
                    width: parent.width * root.level / 100
                    height: parent.height
                    radius: 3
                    color: Theme.accent
                    Behavior on width { NumberAnimation { duration: Theme.motionFast } }
                }
            }
        }
    }
}
