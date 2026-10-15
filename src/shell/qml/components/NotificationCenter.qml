// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Fidan.Shell

GlassPanel {
    id: root
    implicitWidth: 364
    implicitHeight: 484
    raised: true

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            Text {
                Layout.fillWidth: true
                text: qsTr("Notifications")
                color: Theme.text(Controller.darkTheme)
                font.family: Theme.fontFamily
                font.pixelSize: 16
                font.weight: Font.Bold
            }
            Button {
                id: clearButton
                text: qsTr("Clear")
                flat: true
                enabled: Notifications.history.length > 0
                onClicked: Notifications.clearHistory()
                opacity: enabled ? 1 : 0.45
                contentItem: Text {
                    text: clearButton.text; color: Theme.text(Controller.darkTheme)
                    font.family: Theme.fontFamily; font.pixelSize: 12
                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 6
                    color: clearButton.hovered ? (Controller.darkTheme ? "#315F46" : "#C7E5D3") : "transparent"
                }
            }
            IconButton {
                compact: true
                label: "×"
                accessibleName: qsTr("Close notifications")
                onClicked: Notifications.historyVisible = false
            }
        }

        ListView {
            id: historyList
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 8
            clip: true
            model: Notifications.history
            boundsBehavior: Flickable.StopAtBounds

            delegate: Rectangle {
                id: notificationCard
                required property var modelData
                width: historyList.width
                height: Math.max(76, messageColumn.implicitHeight + 24)
                radius: Theme.radiusMedium
                color: Controller.darkTheme ? "#C1222C28" : "#E7F0EB"
                border.width: 1
                border.color: Theme.border(Controller.darkTheme)

                Column {
                    id: messageColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.margins: 12
                    spacing: 4
                    Row {
                        width: parent.width
                        Text {
                            width: parent.width - timeLabel.width - 8
                            text: modelData.appName || qsTr("Application")
                            color: Theme.accent
                            font.family: Theme.fontFamily
                            font.pixelSize: 10
                            font.weight: Font.DemiBold
                            elide: Text.ElideRight
                        }
                        Text {
                            id: timeLabel
                            text: Qt.formatTime(new Date(modelData.receivedAt), "HH:mm")
                            color: Theme.muted(Controller.darkTheme)
                            font.family: Theme.fontFamily
                            font.pixelSize: 9
                        }
                    }
                    Text {
                        width: parent.width
                        text: modelData.summary
                        color: Theme.text(Controller.darkTheme)
                        font.family: Theme.fontFamily
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                        elide: Text.ElideRight
                    }
                    Text {
                        width: parent.width
                        visible: text.length > 0
                        text: modelData.body
                        color: Theme.muted(Controller.darkTheme)
                        font.family: Theme.fontFamily
                        font.pixelSize: 10
                        wrapMode: Text.Wrap
                        maximumLineCount: 3
                        elide: Text.ElideRight
                    }
                    Flow {
                        width: parent.width; spacing: 6
                        visible: notificationCard.modelData.active && notificationCard.modelData.actions && notificationCard.modelData.actions.length > 0
                        Repeater {
                            model: notificationCard.modelData.actions || []
                            delegate: Button {
                                id: notificationAction
                                required property var modelData
                                text: modelData.label || qsTr("Open")
                                onClicked: Notifications.invokeAction(notificationCard.modelData.id, modelData.key)
                                contentItem: Text { text: notificationAction.text; color: Theme.text(Controller.darkTheme); font.pixelSize: 11 }
                                background: Rectangle { radius: 5; color: Controller.darkTheme ? "#315F46" : "#C7E5D3" }
                            }
                        }
                    }
                }
            }

            Label {
                anchors.centerIn: parent
                visible: historyList.count === 0
                text: qsTr("No notifications yet")
                color: Theme.muted(Controller.darkTheme)
                font.family: Theme.fontFamily
            }
        }
    }
}
