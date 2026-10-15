// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import Fidan.Shell

GlassPanel {
    id: root
    implicitWidth: 340
    implicitHeight: Math.max(92, content.implicitHeight + 28)
    raised: true

    Column {
        id: content
        anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
        anchors.margins: 14
        spacing: 6
        Text {
            width: parent.width - 26; text: Notifications.appName; textFormat: Text.PlainText
            color: Controller.darkTheme ? Theme.accent : "#207243"; font.family: Theme.fontFamily
            font.pixelSize: 10; elide: Text.ElideRight
        }
        Text {
            width: parent.width; text: Notifications.summary; textFormat: Text.PlainText
            color: Theme.text(Controller.darkTheme); font.family: Theme.fontFamily
            font.pixelSize: 12; font.weight: Font.DemiBold; wrapMode: Text.Wrap
        }
        Text {
            width: parent.width; text: Notifications.body; textFormat: Text.PlainText
            color: Theme.muted(Controller.darkTheme); font.family: Theme.fontFamily
            font.pixelSize: 11; wrapMode: Text.Wrap; maximumLineCount: 3; elide: Text.ElideRight
        }
        Flow {
            width: parent.width; spacing: 6
            Repeater {
                model: Notifications.actions
                delegate: Button {
                    id: action
                    required property var modelData
                    text: modelData.label || qsTr("Open")
                    onClicked: Notifications.invokeCurrentAction(modelData.key)
                    contentItem: Text { text: action.text; color: Theme.text(Controller.darkTheme); font.pixelSize: 11 }
                    background: Rectangle { radius: 5; color: Controller.darkTheme ? "#315F46" : "#C7E5D3" }
                }
            }
        }
    }
    IconButton {
        anchors.top: parent.top; anchors.right: parent.right; anchors.margins: 5
        compact: true; label: "×"; accessibleName: qsTr("Dismiss notification")
        onClicked: Notifications.dismissCurrent()
    }
}
