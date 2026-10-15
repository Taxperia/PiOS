// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import Fidan.Shell

GlassPanel {
    id: root
    width: 270
    height: menuColumn.height + 14
    radius: 13
    raised: true
    clip: false
    focus: visible

    signal settingsPageRequested(string page)
    signal dismissed

    property bool sortMenuVisible: false
    onVisibleChanged: if (visible) DesktopFiles.refreshClipboard()
    readonly property string sortedBy: DesktopItems.sortMode
    readonly property bool submenuOnLeft: parent && x + width * 2 > parent.width

    function closeMenu() {
        sortMenuVisible = false
        dismissed()
    }

    function notify(summary, body) {
        Notifications.Notify("Project Fidan", 0, "", summary, body, [], {}, 3000)
    }

    function run(action) {
        if (action === "new-folder") {
            const name = Controller.createDesktopFolder()
            if (name.length > 0) notify(qsTr("Folder created"), name)
        } else if (action === "new-file") {
            const name = Controller.createDesktopFile()
            if (name.length > 0) notify(qsTr("File created"), name)
        } else if (action === "paste") {
            DesktopFiles.paste()
        } else if (action === "terminal") {
            Controller.openDesktopTerminal()
        } else if (action === "wallpaper" || action === "personalize") {
            settingsPageRequested("Personalization")
        } else if (action === "display") {
            settingsPageRequested("Display")
        } else if (action === "refresh") {
            DesktopItems.refresh()
            notify(qsTr("Desktop refreshed"), qsTr("Desktop contents are up to date."))
        }
        closeMenu()
    }

    Keys.onEscapePressed: closeMenu()

    component MenuRow: Rectangle {
        id: row
        property string title: ""
        property string shortcut: ""
        property string iconName: "apps"
        property bool enabled: true
        property bool arrow: false
        property bool selected: false
        signal triggered

        width: menuColumn.width
        height: 34
        radius: 7
        color: enabled && (rowMouse.containsMouse || selected) ? "#315F46" : "transparent"
        opacity: enabled ? 1.0 : 0.42

        SystemIcon {
            anchors.left: parent.left
            anchors.leftMargin: 9
            anchors.verticalCenter: parent.verticalCenter
            width: 18
            height: 18
            name: row.iconName
            color: row.enabled && (rowMouse.containsMouse || row.selected) ? Theme.accent : Theme.text(Controller.darkTheme)
        }
        Text {
            anchors.left: parent.left
            anchors.leftMargin: 43
            anchors.verticalCenter: parent.verticalCenter
            text: row.title
            color: Theme.text(Controller.darkTheme)
            font.family: Theme.fontFamily
            font.pixelSize: 11
            font.weight: Font.Medium
        }
        Text {
            anchors.right: parent.right
            anchors.rightMargin: row.arrow ? 28 : 10
            anchors.verticalCenter: parent.verticalCenter
            visible: row.shortcut.length > 0
            text: row.shortcut
            color: Theme.muted(Controller.darkTheme)
            font.family: Theme.fontFamily
            font.pixelSize: 9
        }
        Text {
            anchors.right: parent.right
            anchors.rightMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            visible: row.arrow
            text: "›"
            color: Theme.text(Controller.darkTheme)
            font.family: "sans-serif"
            font.pixelSize: 21
        }
        MouseArea {
            id: rowMouse
            anchors.fill: parent
            enabled: row.enabled
            hoverEnabled: true
            cursorShape: row.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
            onClicked: row.triggered()
        }
        Behavior on color { ColorAnimation { duration: Theme.motionFast } }
    }

    component MenuSeparator: Item {
        width: menuColumn.width
        height: 9
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            color: Theme.border(Controller.darkTheme)
            opacity: 0.65
            height: 1
        }
    }

    Column {
        id: menuColumn
        x: 7
        y: 7
        width: root.width - 14

        MenuRow { title: qsTr("New Folder"); shortcut: "Shift+Ctrl+N"; iconName: "new-folder"; onTriggered: root.run("new-folder") }
        MenuRow { title: qsTr("New File"); iconName: "file"; onTriggered: root.run("new-file") }
        MenuSeparator {}
        MenuRow { title: qsTr("Paste"); shortcut: "Ctrl+V"; iconName: "clipboard"; enabled: DesktopFiles.canPaste && !DesktopFiles.busy; onTriggered: root.run("paste") }
        MenuSeparator {}
        MenuRow { title: qsTr("Open in Terminal"); shortcut: "Ctrl+Alt+T"; iconName: "terminal"; onTriggered: root.run("terminal") }
        MenuSeparator {}
        MenuRow { title: qsTr("Change Wallpaper"); iconName: "picture"; onTriggered: root.run("wallpaper") }
        MenuRow { title: qsTr("Display Settings"); iconName: "display"; onTriggered: root.run("display") }
        MenuRow {
            title: qsTr("Sort By")
            iconName: "sort"
            arrow: true
            selected: root.sortMenuVisible
            onTriggered: root.sortMenuVisible = !root.sortMenuVisible
        }
        MenuRow { title: qsTr("Refresh"); shortcut: "F5"; iconName: "refresh"; onTriggered: root.run("refresh") }
        MenuSeparator {}
        MenuRow { title: qsTr("Personalize"); iconName: "personalize"; arrow: true; onTriggered: root.run("personalize") }
    }

    GlassPanel {
        id: sortMenu
        visible: root.sortMenuVisible
        width: 168
        height: sortColumn.height + 14
        x: root.submenuOnLeft ? -width + 6 : root.width - 6
        y: 222
        radius: 11
        raised: true
        z: 3

        Column {
            id: sortColumn
            x: 7
            y: 7
            width: parent.width - 14
            Repeater {
                model: ["Name", "Type", "Date Modified"]
                Rectangle {
                    required property string modelData
                    width: sortColumn.width
                    height: 32
                    radius: 7
                    color: sortMouse.containsMouse ? "#315F46" : "transparent"
                    Text { anchors.left: parent.left; anchors.leftMargin: 10; anchors.verticalCenter: parent.verticalCenter; text: modelData; color: Theme.text(Controller.darkTheme); font.family: Theme.fontFamily; font.pixelSize: 10 }
                    Text { anchors.right: parent.right; anchors.rightMargin: 10; anchors.verticalCenter: parent.verticalCenter; visible: root.sortedBy === modelData; text: "✓"; color: Theme.accent; font.pixelSize: 12 }
                    MouseArea {
                        id: sortMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            DesktopItems.sortMode = modelData
                            root.notify(qsTr("Desktop sorted"), modelData)
                            root.closeMenu()
                        }
                    }
                }
            }
        }
    }
}
