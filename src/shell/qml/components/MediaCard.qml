// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Fidan.Shell

Rectangle {
    id: root
    readonly property var track: DesktopServices.media
    readonly property bool available: DesktopServices.mediaService.length > 0
    implicitHeight: content.implicitHeight + 24
    radius: 12
    color: Controller.darkTheme ? "#24352D" : "#E1EBE6"
    ColumnLayout {
        id: content
        anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
        anchors.margins: 12
        spacing: 8
        RowLayout {
            Layout.fillWidth: true
            Rectangle {
                Layout.preferredWidth: 54; Layout.preferredHeight: 54
                radius: 10; color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.15); clip: true
                SystemIcon { anchors.centerIn: parent; width: 26; height: 26; name: "music"; color: Theme.accent; visible: cover.status !== Image.Ready }
                Image { id: cover; anchors.fill: parent; source: root.available ? root.track.art || "" : ""; asynchronous: true; fillMode: Image.PreserveAspectCrop; visible: status === Image.Ready }
            }
            ColumnLayout {
                Layout.fillWidth: true; spacing: 4
                Text { Layout.fillWidth: true; text: root.available ? root.track.title || qsTr("Unknown track") : qsTr("Nothing playing"); elide: Text.ElideRight; color: Theme.text(Controller.darkTheme); font.family: Theme.fontFamily; font.pixelSize: 12; font.weight: Font.DemiBold }
                Text { Layout.fillWidth: true; text: root.available ? root.track.artist || root.track.player || "" : qsTr("Open a music or video player"); elide: Text.ElideRight; color: Theme.muted(Controller.darkTheme); font.family: Theme.fontFamily; font.pixelSize: 10 }
            }
        }
        ComboBox {
            Layout.fillWidth: true; visible: DesktopServices.mediaPlayers.length > 1
            model: DesktopServices.mediaPlayers; textRole: "name"; valueRole: "id"
            currentIndex: count > 0 ? DesktopServices.mediaPlayers.findIndex(player => player.id === DesktopServices.mediaService) : -1
            onActivated: index => DesktopServices.selectPlayer(DesktopServices.mediaPlayers[index].id)
            Accessible.name: qsTr("Media player")
            palette.button: Theme.raised(Controller.darkTheme); palette.buttonText: Theme.text(Controller.darkTheme)
            palette.base: Theme.raised(Controller.darkTheme); palette.text: Theme.text(Controller.darkTheme)
            palette.window: Theme.raised(Controller.darkTheme); palette.highlight: Theme.accentStrong
        }
        RowLayout {
            visible: root.available; Layout.alignment: Qt.AlignHCenter; spacing: 18
            IconButton { compact: true; iconName: "previous"; enabled: root.track.canPrevious || false; opacity: enabled ? 1 : 0.35; accessibleName: qsTr("Previous track"); onClicked: DesktopServices.mediaAction("Previous") }
            IconButton {
                objectName: "mediaPlayPause"
                iconName: root.track.playing ? "pause" : "media-play"; active: root.track.playing || false
                enabled: root.track.playing ? root.track.canPause || false : root.track.canPlay || false
                opacity: enabled ? 1 : 0.35
                accessibleName: root.track.playing ? qsTr("Pause") : qsTr("Play")
                onClicked: DesktopServices.mediaAction("PlayPause")
            }
            IconButton { compact: true; iconName: "next"; enabled: root.track.canNext || false; opacity: enabled ? 1 : 0.35; accessibleName: qsTr("Next track"); onClicked: DesktopServices.mediaAction("Next") }
        }
    }
}
