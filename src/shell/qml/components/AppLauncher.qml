// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Fidan.Shell

Rectangle {
    id: root
    objectName: "appLauncher"
    implicitWidth: 800; implicitHeight: 720
    radius: 14; antialiasing: true
    color: Controller.darkTheme ? "#F2101A1D" : "#F6F3F8F5"
    border.color: Controller.darkTheme ? "#66869B99" : "#8BABB6AC"
    property string page: "home"
    property string pendingPowerAction: ""
    readonly property bool searching: search.text.trim().length > 0
    readonly property bool homePage: page === "home" && !searching
    readonly property color foreground: Theme.text(Controller.darkTheme)
    readonly property color muted: Theme.muted(Controller.darkTheme)
    readonly property color linkColor: Controller.darkTheme ? Theme.accent : Qt.darker(Theme.accent, 1.8)
    readonly property color hairline: Controller.darkTheme ? "#253E4545" : "#26405B49"
    readonly property var matchingFiles: Applications.recentFiles.filter(file => !searching || (file.name + " " + file.folder).toLowerCase().includes(search.text.trim().toLowerCase()))

    function failed() {
        Notifications.Notify("PiOS", 0, "dialog-warning", qsTr("Could not open this item"), qsTr("It may have been moved or uninstalled. Refresh the menu and try again."), [], {}, 5000)
    }
    function launch(id) {
        if (Applications.launchDesktopId(id)) Controller.launcherVisible = false
        else failed()
    }
    function launchRow(row) {
        if (Applications.launch(row)) Controller.launcherVisible = false
        else failed()
    }
    function selectCategory(category) {
        page = "apps"; search.clear(); Applications.category = category; body.contentY = 0
    }
    function openRecent(row) {
        if (Applications.openRecentFile(row)) Controller.launcherVisible = false
        else failed()
    }
    function appMenu(id, row, pinned) {
        applicationMenu.desktopId = id; applicationMenu.appRow = row; applicationMenu.isPinned = pinned; applicationMenu.popup()
    }
    function reset() {
        page = "home"; search.clear(); Applications.category = ""; Applications.filter = ""; Applications.refresh(); body.contentY = 0; search.forceActiveFocus()
    }
    Connections {
        target: Controller
        function onLauncherVisibleChanged() {
            if (Controller.launcherVisible) root.reset()
            else { powerMenu.close(); applicationMenu.close(); powerDialog.close() }
        }
    }
    Keys.onEscapePressed: {
        if (search.text.length) search.clear()
        else Controller.launcherVisible = false
    }
    Shortcut { sequence: StandardKey.Find; enabled: root.visible; onActivated: { search.forceActiveFocus(); search.selectAll() } }
    component LabelText: Text {
        color: root.foreground; font.family: Theme.fontFamily; font.pixelSize: 13; elide: Text.ElideRight
    }
    component NavigationButton: Button {
        id: navigation
        property string symbol: "apps"
        property bool selected: false
        property bool trailing: false
        width: sidebar.width - 20; height: 38
        hoverEnabled: true; leftPadding: 16; rightPadding: 12
        background: Rectangle {
            radius: 8
            color: navigation.selected ? (Controller.darkTheme ? "#CC234D38" : "#C9E8D6") : navigation.hovered || navigation.activeFocus ? (Controller.darkTheme ? "#304D6156" : "#DCEAE2") : "transparent"
            border.color: navigation.activeFocus ? Theme.accent : "transparent"
            Rectangle { visible: navigation.selected; width: 2; height: 26; radius: 1; anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter; color: Theme.accent }
        }
        contentItem: RowLayout {
            spacing: 16
            SystemIcon { Layout.preferredWidth: 19; Layout.preferredHeight: 19; name: navigation.symbol; color: root.foreground }
            LabelText { Layout.fillWidth: true; text: navigation.text }
            LabelText { visible: navigation.trailing; text: "›"; font.pixelSize: 21 }
        }
    }
    component AppTile: Button {
        id: tile
        property string appName: ""
        property string appIcon: ""
        property string desktopId: ""
        property int appRow: -1
        property int pinIndex: -1
        property bool pinned: true
        width: 128; height: 100; hoverEnabled: true
        Accessible.name: appName
        onClicked: root.launch(desktopId)
        ToolTip.visible: hovered; ToolTip.delay: 800
        ToolTip.text: appName + "\n" + qsTr("Right-click for pin and desktop options")
        background: Rectangle {
            radius: 9
            gradient: Gradient {
                GradientStop { position: 0; color: Controller.darkTheme ? (tile.hovered ? "#D52B3D3C" : "#AA233031") : (tile.hovered ? "#F5FFFFFF" : "#EDF5F0") }
                GradientStop { position: 1; color: Controller.darkTheme ? (tile.hovered ? "#C620302C" : "#99172323") : (tile.hovered ? "#E1F0E7" : "#E6EEE9") }
            }
            border.color: tile.activeFocus ? Theme.accent : root.hairline; border.width: tile.activeFocus ? 2 : 1
        }
        contentItem: Column {
            spacing: 9; topPadding: 9
            Image {
                anchors.horizontalCenter: parent.horizontalCenter; width: 48; height: 48
                source: "image://theme/" + encodeURIComponent(tile.appIcon) + (Controller.darkTheme ? "?theme=dark" : "?theme=light")
                sourceSize: Qt.size(96, 96); fillMode: Image.PreserveAspectFit; asynchronous: true
            }
            LabelText { width: parent.width; horizontalAlignment: Text.AlignHCenter; text: tile.appName; font.pixelSize: 12 }
        }
        TapHandler { acceptedButtons: Qt.RightButton; onTapped: root.appMenu(tile.desktopId, tile.appRow, tile.pinned) }
        Keys.onMenuPressed: root.appMenu(desktopId, appRow, pinned)
        Keys.onPressed: event => {
            if (pinIndex >= 0) {
                const delta = event.key === Qt.Key_Right ? 1 : event.key === Qt.Key_Left ? -1 : event.key === Qt.Key_Down ? 4 : event.key === Qt.Key_Up ? -4 : 0
                if (delta) {
                    const next = pinnedRepeater.itemAt(pinIndex + delta)
                    if (next) next.forceActiveFocus()
                    else if (pinIndex + delta < 0) search.forceActiveFocus()
                    event.accepted = true
                }
            }
        }
    }
    component SectionHeading: RowLayout {
        id: heading
        property string title: ""
        property string actionText: ""
        signal activated
        height: 22
        LabelText { Layout.fillWidth: true; text: parent.title; font.pixelSize: 15; font.weight: Font.DemiBold }
        Button {
            id: headingAction
            visible: heading.actionText.length > 0; text: heading.actionText + "  →"; flat: true; padding: 0
            contentItem: LabelText { text: headingAction.text; color: root.linkColor; font.pixelSize: 11; verticalAlignment: Text.AlignVCenter }
            background: Rectangle { radius: 4; color: headingAction.hovered || headingAction.activeFocus ? "#224FAE72" : "transparent" }
            onClicked: heading.activated()
        }
    }
    component LauncherMenu: Menu {
        palette.window: Controller.darkTheme ? "#182724" : "#F4F8F5"
        palette.base: Controller.darkTheme ? "#182724" : "#F4F8F5"
        palette.text: root.foreground
        palette.windowText: root.foreground
        palette.buttonText: root.foreground
        palette.highlight: Theme.accentStrong
        background: Rectangle { radius: 9; color: Controller.darkTheme ? "#182724" : "#F4F8F5"; border.color: root.hairline }
    }
    // Translucent tint without the cost of a continuous blur pass.
    Rectangle {
        anchors.fill: parent; anchors.margins: 1; radius: 13
        gradient: Gradient {
            GradientStop { position: 0; color: Controller.darkTheme ? "#352D4A50" : "#30FFFFFF" }
            GradientStop { position: 0.55; color: "transparent" }
            GradientStop { position: 1; color: Controller.darkTheme ? "#55000805" : "#20CBE2D3" }
        }
    }
    Item {
        id: sidebar
        width: 196; anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom; anchors.margins: 1
        Rectangle { anchors.fill: parent; anchors.margins: 1; radius: 12; color: Controller.darkTheme ? "#58111A1B" : "#559BAFA2" }
        Rectangle { width: 1; anchors.right: parent.right; anchors.top: parent.top; anchors.bottom: parent.bottom; color: root.hairline }
        RowLayout {
            anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right
            anchors.topMargin: 26; anchors.leftMargin: 22; anchors.rightMargin: 16; spacing: 14
            Rectangle {
                Layout.preferredWidth: 46; Layout.preferredHeight: 46; radius: 23
                color: Controller.darkTheme ? "#344043" : "#DFEAE4"; border.color: Controller.darkTheme ? "#A1AFAF" : "#7B9484"
                SystemIcon { anchors.centerIn: parent; width: 35; height: 35; name: "user"; color: Controller.darkTheme ? "#D5DCDA" : "#486654" }
            }
            ColumnLayout {
                Layout.fillWidth: true; spacing: 3
                LabelText { Layout.fillWidth: true; text: Applications.userName; font.pixelSize: 14; font.weight: Font.DemiBold }
                LabelText { Layout.fillWidth: true; text: Applications.userHandle; font.pixelSize: 11; color: root.muted }
            }
        }
        Column {
            anchors.left: parent.left; anchors.leftMargin: 9; anchors.right: parent.right; anchors.top: parent.top; anchors.topMargin: 91
            Repeater {
                model: [
                    { title: qsTr("All Apps"), category: "", icon: "grid" },
                    { title: qsTr("Accessories"), category: "Utility", icon: "display" },
                    { title: qsTr("Development"), category: "Development", icon: "code" },
                    { title: qsTr("Education"), category: "Education", icon: "education" },
                    { title: qsTr("Graphics"), category: "Graphics", icon: "palette" },
                    { title: qsTr("Internet"), category: "Network", icon: "globe" },
                    { title: qsTr("Multimedia"), category: "AudioVideo", icon: "media" },
                    { title: qsTr("Office"), category: "Office", icon: "file" },
                    { title: qsTr("System Tools"), category: "System", icon: "settings" },
                    { title: qsTr("Hardware"), category: "HardwareSettings", icon: "chip" },
                    { title: qsTr("Preferences"), category: "Settings", icon: "preferences" }
                ]
                NavigationButton {
                    required property var modelData
                    objectName: "launcher-category-" + (modelData.category || "All")
                    text: modelData.title; symbol: modelData.icon
                    selected: root.page !== "recent" && Applications.category === modelData.category
                    onClicked: root.selectCategory(modelData.category)
                }
            }
        }
        Column {
            anchors.left: parent.left; anchors.leftMargin: 9; anchors.bottom: parent.bottom; anchors.bottomMargin: 50; spacing: 2
            Rectangle { width: sidebar.width - 40; height: 1; x: 10; color: root.hairline }
            Item { width: 1; height: 8 }
            NavigationButton { objectName: "launcher-pinned"; text: qsTr("Pinned Apps"); symbol: "pin"; selected: Applications.category === "Pinned" && root.page === "apps"; onClicked: root.selectCategory("Pinned") }
            NavigationButton { objectName: "launcher-recent"; text: qsTr("Recent Files"); symbol: "recent"; selected: root.page === "recent"; onClicked: { root.page = "recent"; search.clear(); body.contentY = 0 } }
            NavigationButton { objectName: "launcher-power"; text: qsTr("Power"); symbol: "power"; trailing: true; onClicked: powerMenu.popup(this, width, -powerMenu.height + height) }
        }
        LabelText { anchors.left: parent.left; anchors.leftMargin: 27; anchors.bottom: parent.bottom; anchors.bottomMargin: 18; text: "PiOS"; font.pixelSize: 10; font.letterSpacing: 2.5; color: root.muted; opacity: 0.55 }
    }
    RowLayout {
        id: searchRow
        anchors.left: sidebar.right; anchors.right: parent.right; anchors.top: parent.top
        anchors.leftMargin: 20; anchors.rightMargin: 22; anchors.topMargin: 21; spacing: 12
        TextField {
            id: search
            objectName: "launcher-search"
            Layout.fillWidth: true; Layout.preferredHeight: 42; leftPadding: 44; rightPadding: 34
            placeholderText: qsTr("Search apps, files, settings, and more…")
            color: root.foreground; placeholderTextColor: root.muted; font.family: Theme.fontFamily; font.pixelSize: 12; selectByMouse: true
            background: Rectangle { radius: 10; color: Controller.darkTheme ? "#4D293739" : "#99FFFFFF"; border.color: search.activeFocus ? "#7D699388" : root.hairline }
            SystemIcon { anchors.left: parent.left; anchors.leftMargin: 14; anchors.verticalCenter: parent.verticalCenter; width: 19; height: 19; name: "search"; color: root.foreground }
            Button {
                id: clearSearch
                visible: search.text.length > 0; anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; width: 30; height: 30; text: "×"; flat: true
                Accessible.name: qsTr("Clear search"); onClicked: search.clear()
                contentItem: LabelText { text: clearSearch.text; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; font.pixelSize: 19 }
                background: Rectangle { color: "transparent" }
            }
            onTextChanged: Applications.filter = text
            onAccepted: {
                if (root.page === "recent") { if (root.matchingFiles.length) root.openRecent(root.matchingFiles[0].row) }
                else if (apps.count) root.launchRow(Math.min(apps.count - 1, Math.max(0, apps.currentIndex)))
                else if (root.matchingFiles.length) root.openRecent(root.matchingFiles[0].row)
            }
            Keys.onDownPressed: {
                if (root.homePage && pinnedRepeater.count) pinnedRepeater.itemAt(0).forceActiveFocus()
                else if (apps.count) { apps.currentIndex = 0; apps.forceActiveFocus() }
            }
        }
        Button {
            id: preferences
            objectName: "launcher-preferences"; Layout.preferredWidth: 36; Layout.preferredHeight: 36; hoverEnabled: true; padding: 9
            Accessible.name: qsTr("Open settings"); ToolTip.visible: hovered; ToolTip.text: qsTr("Open settings")
            contentItem: SystemIcon { name: "preferences"; color: root.foreground }
            background: Rectangle { radius: 8; color: preferences.hovered ? "#284D6A59" : "transparent"; border.color: root.hairline }
            onClicked: { Controller.openSettings(); Controller.launcherVisible = false }
        }
    }
    Flickable {
        id: body
        anchors.left: sidebar.right; anchors.right: parent.right; anchors.top: searchRow.bottom; anchors.bottom: parent.bottom
        anchors.leftMargin: 20; anchors.rightMargin: 22; anchors.topMargin: 18; anchors.bottomMargin: 20
        contentWidth: width; contentHeight: sections.height; boundsBehavior: Flickable.StopAtBounds; clip: true
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
        Column {
            id: sections
            width: body.width; spacing: 16
            Column {
                width: parent.width; spacing: 10; visible: root.homePage
                SectionHeading { width: parent.width; title: qsTr("Pinned Apps"); actionText: qsTr("All Apps"); onActivated: root.selectCategory("") }
                Grid {
                    id: pinnedGrid
                    width: parent.width; columns: 4; spacing: 12
                    Repeater {
                        id: pinnedRepeater
                        model: Applications.pinnedApplications.slice(0, 8)
                        AppTile {
                            required property int index
                            required property var modelData
                            objectName: "launcher-pin-" + modelData.desktopId
                            width: (pinnedGrid.width - 36) / 4; appName: modelData.name; appIcon: modelData.iconName; desktopId: modelData.desktopId; pinIndex: index
                        }
                    }
                }
                LabelText { visible: Applications.pinnedApplications.length === 0; width: parent.width; height: 80; wrapMode: Text.WordWrap; color: root.muted; text: qsTr("Right-click an app in All Apps to pin it here.") }
            }
            Column {
                width: parent.width; spacing: 10; visible: root.homePage
                SectionHeading { width: parent.width; title: qsTr("Recommended"); actionText: qsTr("See All"); onActivated: { Controller.openSoftwareCenter(); Controller.launcherVisible = false } }
                Row {
                    width: parent.width; spacing: 10
                    Repeater {
                        model: [
                            { title: qsTr("Getting Started"), detail: qsTr("Make PiOS your own"), icon: "org.fidan.Settings", action: "settings", tint: "#285748" },
                            { title: qsTr("Discover Apps"), detail: qsTr("Tools for work and play"), icon: "org.fidan.Store", action: "store", tint: "#263C66" },
                            { title: qsTr("For Makers"), detail: qsTr("Explore your system"), icon: "org.fidan.TaskManager", action: "tasks", tint: "#69532A" }
                        ]
                        Button {
                            id: recommendation
                            required property var modelData
                            width: (parent.width - 20) / 3; height: 76; hoverEnabled: true; padding: 10; Accessible.name: modelData.title
                            background: Rectangle { radius: 8; color: Controller.darkTheme ? (recommendation.hovered ? "#B02A3B35" : "#881B2827") : (recommendation.hovered ? "#FFFFFF" : "#E7EFEB"); border.color: recommendation.activeFocus ? Theme.accent : root.hairline }
                            contentItem: RowLayout {
                                spacing: 10
                                Rectangle {
                                    Layout.preferredWidth: 35; Layout.preferredHeight: 36; radius: 7; color: recommendation.modelData.tint
                                    Image { anchors.centerIn: parent; width: 27; height: 27; source: "image://theme/" + recommendation.modelData.icon; sourceSize: Qt.size(64, 64) }
                                }
                                ColumnLayout {
                                    Layout.fillWidth: true; spacing: 4
                                    LabelText { Layout.fillWidth: true; text: recommendation.modelData.title; font.pixelSize: 11 }
                                    LabelText { Layout.fillWidth: true; text: recommendation.modelData.detail; color: root.muted; font.pixelSize: 10; wrapMode: Text.WordWrap; maximumLineCount: 2 }
                                }
                            }
                            onClicked: {
                                if (modelData.action === "settings") Controller.openSettings()
                                else if (modelData.action === "store") Controller.openSoftwareCenter()
                                else Controller.openTaskManager()
                                Controller.launcherVisible = false
                            }
                        }
                    }
                }
            }
            Column {
                width: parent.width; spacing: 10; visible: !root.homePage && root.page !== "recent"
                SectionHeading { width: parent.width; title: root.searching ? qsTr("Search Results") : Applications.category === "Pinned" ? qsTr("Pinned Apps") : qsTr("Applications"); actionText: qsTr("Home"); onActivated: root.reset() }
                GridView {
                    id: apps
                    objectName: "launcher-app-grid"
                    width: parent.width; height: Math.max(210, body.height - 32 - (root.searching && root.matchingFiles.length ? 192 : 0))
                    cellWidth: width / 4; cellHeight: 112; model: Applications
                    clip: true; keyNavigationEnabled: true; activeFocusOnTab: true
                    ScrollBar.vertical: ScrollBar {}
                    delegate: AppTile {
                        required property int index
                        required property string name
                        required property string iconName
                        required desktopId
                        required pinned
                        appName: name; appIcon: iconName; appRow: index; width: apps.cellWidth - 12
                    }
                    Keys.onReturnPressed: root.launchRow(currentIndex)
                    Keys.onEnterPressed: root.launchRow(currentIndex)
                    LabelText { visible: apps.count === 0; anchors.centerIn: parent; width: parent.width - 40; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap; text: qsTr("No applications match your search"); color: root.muted }
                }
            }
            Column {
                width: parent.width; spacing: 8
                visible: root.homePage || root.page === "recent" || (root.searching && root.matchingFiles.length > 0)
                SectionHeading {
                    width: parent.width; title: root.page === "recent" ? qsTr("Recent Files") : qsTr("Recent"); actionText: root.page === "recent" ? qsTr("Home") : qsTr("See All")
                    onActivated: { if (root.page === "recent") root.reset(); else { root.page = "recent"; search.clear(); body.contentY = 0 } }
                }
                Rectangle {
                    width: parent.width; height: recentRows.height; radius: 8
                    color: Controller.darkTheme ? "#661E2D29" : "#77E1EDE5"; border.color: root.hairline
                    Column {
                        id: recentRows
                        width: parent.width
                        Repeater {
                            model: root.matchingFiles.slice(0, root.page === "recent" ? 20 : root.searching ? 3 : 5)
                            Button {
                                id: recent
                                required property var modelData
                                width: parent.width; height: 40; padding: 9; hoverEnabled: true
                                objectName: "launcher-recent-file-" + modelData.row; Accessible.name: modelData.name
                                background: Rectangle {
                                    radius: 6; color: recent.hovered || recent.activeFocus ? "#284FAE72" : "transparent"
                                    Rectangle { anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.right: parent.right; anchors.leftMargin: 10; anchors.rightMargin: 10; height: 1; color: root.hairline }
                                }
                                contentItem: RowLayout {
                                    spacing: 12
                                    Image { Layout.preferredWidth: 24; Layout.preferredHeight: 26; source: "image://theme/" + encodeURIComponent(recent.modelData.iconName); sourceSize: Qt.size(48, 48); fillMode: Image.PreserveAspectFit }
                                    ColumnLayout {
                                        Layout.fillWidth: true; spacing: 1
                                        LabelText { Layout.fillWidth: true; text: recent.modelData.name; font.pixelSize: 11 }
                                        LabelText { Layout.fillWidth: true; text: recent.modelData.folder; color: root.muted; font.pixelSize: 10 }
                                    }
                                    LabelText { text: recent.modelData.date; color: root.muted; font.pixelSize: 10 }
                                }
                                onClicked: root.openRecent(modelData.row)
                            }
                        }
                        LabelText {
                            visible: root.matchingFiles.length === 0; width: parent.width; height: 110; padding: 16
                            horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; wrapMode: Text.WordWrap; color: root.muted; font.pixelSize: 12
                            text: root.searching ? qsTr("No recent files match your search") : qsTr("Your recently opened files will appear here.")
                        }
                    }
                }
            }
        }
    }
    LauncherMenu {
        id: applicationMenu
        property string desktopId: ""
        property int appRow: -1
        property bool isPinned: false
        MenuItem { text: applicationMenu.isPinned ? qsTr("Unpin from Start") : qsTr("Pin to Start"); onTriggered: Applications.togglePinned(applicationMenu.desktopId) }
        MenuItem { text: qsTr("Add to desktop"); visible: applicationMenu.appRow >= 0; height: visible ? implicitHeight : 0; onTriggered: { if (Applications.addToDesktop(applicationMenu.appRow)) DesktopItems.refresh(); else root.failed() } }
    }
    LauncherMenu {
        id: powerMenu
        MenuItem { text: qsTr("Lock"); enabled: Controller.lockAvailable; onTriggered: { Controller.lockSession(); Controller.launcherVisible = false } }
        MenuSeparator {}
        MenuItem { text: qsTr("Restart…"); enabled: Controller.powerActionsAvailable; onTriggered: { root.pendingPowerAction = "restart"; powerDialog.open() } }
        MenuItem { text: qsTr("Shut Down…"); enabled: Controller.powerActionsAvailable; onTriggered: { root.pendingPowerAction = "shutdown"; powerDialog.open() } }
    }
    Dialog {
        id: powerDialog
        palette.window: Controller.darkTheme ? "#182724" : "#F4F8F5"
        palette.windowText: root.foreground
        palette.text: root.foreground
        palette.button: Controller.darkTheme ? "#294438" : "#DEEBE3"
        palette.buttonText: root.foreground
        anchors.centerIn: parent; width: 340; modal: true
        title: root.pendingPowerAction === "restart" ? qsTr("Restart this device?") : qsTr("Shut down this device?")
        standardButtons: Dialog.Cancel | Dialog.Ok
        Label { width: parent.width; text: qsTr("Save your work before continuing."); wrapMode: Text.WordWrap }
        onAccepted: { Controller.requestPowerAction(root.pendingPowerAction); Controller.launcherVisible = false }
    }
}
