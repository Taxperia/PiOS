// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import Fidan.Shell

GlassPanel {
    id: root
    component DockButton: IconButton { width: Controller.dockIconSize+18; height: width; visualIconSize: Controller.dockIconSize }
    property bool horizontal: Controller.dockPosition === "bottom"
    width: dockItems.implicitWidth + 12
    implicitHeight: dockItems.implicitHeight + 20
    height: implicitHeight
    clip: true
    radius: 14
    raised: true

    Grid {
        id: dockItems
        anchors.centerIn: parent
        columns: root.horizontal ? 10 : 1
        horizontalItemAlignment: Grid.AlignHCenter
        verticalItemAlignment: Grid.AlignVCenter
        spacing: 6

        DockButton { iconName: "home"; active: true; accessibleName: qsTr("Home"); onClicked: Controller.openFileManager() }
        DockButton { iconName: "terminal"; accessibleName: qsTr("Terminal"); onClicked: Controller.openTerminal() }
        DockButton { iconName: "folder"; iconColor: "#69AFFF"; accessibleName: qsTr("Files"); onClicked: Controller.openFileManager() }
        DockButton { iconName: "globe"; iconColor: "#6CB6FF"; accessibleName: qsTr("Web Browser"); onClicked: Controller.launchPinned("chromium.desktop") }
        DockButton { iconName: "store"; iconColor: "#8ADB57"; active: Controller.softwareCenterVisible; accessibleName: qsTr("Software Center"); onClicked: Controller.openSoftwareCenter() }
        DockButton { iconName: "play"; iconColor: "#69D950"; accessibleName: qsTr("Media"); onClicked: Controller.launchPinned("vlc.desktop") }
        DockButton { iconName: "chip"; iconColor: "#A9F0B2"; active: Controller.settingsVisible; accessibleName: qsTr("Settings"); onClicked: Controller.openSettings() }
        DockButton { iconName: "temperature"; iconColor: "#70CDE3"; accessibleName: qsTr("Task Manager"); onClicked: Controller.openTaskManager() }

        Rectangle {
            width: root.horizontal ? 1 : 28
            height: root.horizontal ? 28 : 1
            color: Theme.border(Controller.darkTheme)
        }

        DockButton {
            iconName: "apps"
            accessibleName: qsTr("All applications")
            active: Controller.launcherVisible
            onClicked: Controller.toggleLauncher()
        }
    }
}
