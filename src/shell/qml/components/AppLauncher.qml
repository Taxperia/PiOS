// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import Fidan.Shell

GlassPanel {
    id: root
    implicitWidth: 740
    implicitHeight: 520
    raised: true

    function launchApp(row) {
        if (row < 0 || row >= grid.count) return
        if (Applications.launch(row)) {
            Controller.launcherVisible = false
        } else {
            Notifications.Notify("Project Fidan", 0, "dialog-warning", qsTr("Application could not be started"), qsTr("Check that the application is installed and try again."), [], {}, 5000)
        }
    }

    Connections {
        target: Controller
        function onLauncherVisibleChanged() {
            if (Controller.launcherVisible) {
                Applications.refresh()
                search.forceActiveFocus()
            }
        }
    }

    Column {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 16

        Row {
            id: searchRow
            width: parent.width
            height: 40
            TextField {
                id: search
                width: parent.width - close.width - 10
                height: 40
                placeholderText: qsTr("Search apps and tools…")
                color: Theme.text(Controller.darkTheme)
                placeholderTextColor: Theme.muted(Controller.darkTheme)
                font.family: Theme.fontFamily
                background: Rectangle {
                    radius: 10
                    color: Controller.darkTheme ? "#C8212A27" : "#E9F0EC"
                    border.width: search.activeFocus ? 1 : 0
                    border.color: Theme.accent
                }
                onTextChanged: Applications.filter = text
                onAccepted: root.launchApp(grid.currentIndex)
                Component.onCompleted: forceActiveFocus()
                Keys.onEscapePressed: Controller.launcherVisible = false
                Keys.onDownPressed: grid.forceActiveFocus()
            }
            IconButton { id: close; compact: true; label: "×"; onClicked: Controller.launcherVisible = false }
        }

        Text {
            height: 20
            text: qsTr("Applications")
            color: Theme.text(Controller.darkTheme)
            font.family: Theme.fontFamily
            font.pixelSize: 17
            font.weight: Font.Bold
        }

        Row {
            id: quickActions
            width: parent.width
            height: 70
            spacing: 10

            component BuiltInAction: Rectangle {
                id: action
                property string title: ""
                property string iconName: "apps"
                signal triggered
                width: (quickActions.width - quickActions.spacing * 3) / 4; height: 70; radius: 11
                color: Controller.darkTheme
                    ? (actionMouse.containsMouse ? "#35463F" : "#B9212C28")
                    : (actionMouse.containsMouse ? "#C7E5D3" : "#E1EBE6")
                border.width: 1; border.color: Theme.border(Controller.darkTheme)
                Column {
                    anchors.centerIn: parent; spacing: 6
                    SystemIcon { anchors.horizontalCenter: parent.horizontalCenter; width: 24; height: 24; name: action.iconName; color: Theme.accent }
                    Text { anchors.horizontalCenter: parent.horizontalCenter; text: action.title; color: Theme.text(Controller.darkTheme); font.family: Theme.fontFamily; font.pixelSize: 10 }
                }
                MouseArea { id: actionMouse; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: action.triggered() }
            }

            BuiltInAction { title: qsTr("Files"); iconName: "folder"; onTriggered: Controller.openFileManager() }
            BuiltInAction { title: qsTr("Settings"); iconName: "settings"; onTriggered: Controller.openSettings() }
            BuiltInAction { title: qsTr("Software"); iconName: "store"; onTriggered: Controller.openSoftwareCenter() }
            BuiltInAction { title: qsTr("Tasks"); iconName: "temperature"; onTriggered: Controller.openTaskManager() }
        }

        GridView {
            id: grid
            width: parent.width
            height: parent.height - 178
            cellWidth: 136
            cellHeight: 118
            clip: true
            model: Applications
            activeFocusOnTab: true
            keyNavigationEnabled: true
            Keys.onReturnPressed: {
                root.launchApp(currentIndex)
            }
            Keys.onEnterPressed: {
                root.launchApp(currentIndex)
            }
            Keys.onEscapePressed: Controller.launcherVisible = false
            Text {
                anchors.centerIn: parent
                visible: grid.count === 0
                text: qsTr("No applications match your search")
                color: Theme.muted(Controller.darkTheme)
                font.family: Theme.fontFamily
                font.pixelSize: 13
            }
            delegate: Rectangle {
                required property int index
                required property string name
                required property string comment
                required property string iconName
                width: 126
                height: 106
                radius: 12
                color: appMouse.containsMouse ? (Controller.darkTheme ? "#D42D3934" : "#E1EBE6") : "transparent"
                border.width: GridView.isCurrentItem && grid.activeFocus ? 2 : 0
                border.color: Theme.accent
                Accessible.role: Accessible.Button
                Accessible.name: name
                Column {
                    anchors.centerIn: parent
                    width: parent.width - 14
                    spacing: 7
                    Rectangle {
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: 48; height: 48; radius: 12
                        color: "#315C45"
                        Image {
                            anchors.fill: parent
                            anchors.margins: 6
                            source: "image://theme/" + encodeURIComponent(iconName) + (Controller.darkTheme ? "?theme=dark" : "?theme=light")
                            cache: false
                            sourceSize: Qt.size(40, 40)
                            fillMode: Image.PreserveAspectFit
                            asynchronous: true
                        }
                    }
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        text: name
                        elide: Text.ElideRight
                        color: Theme.text(Controller.darkTheme)
                        font.family: Theme.fontFamily
                        font.pixelSize: 11
                    }
                }
                MouseArea {
                    id: appMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        root.launchApp(index)
                    }
                }
                Rectangle {
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 5
                    width: 24
                    height: 24
                    radius: 7
                    color: desktopMouse.containsMouse ? Theme.accent : "#C52A3A33"
                    Text { anchors.centerIn: parent; text: "+"; color: "white"; font.pixelSize: 16; font.bold: true }
                    MouseArea {
                        id: desktopMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            const added = Applications.addToDesktop(index)
                            if (added) {
                                DesktopItems.refresh()
                                Notifications.Notify("Project Fidan", 0, "", qsTr("Added to desktop"), name, [], {}, 2500)
                            }
                        }
                    }
                }
            }
        }
    }
}

