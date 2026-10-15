// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import QtCore

ApplicationWindow {
    id: root
    width: Math.max(minimumWidth, Math.min(920, Screen.width * 0.78))
    height: Math.max(minimumHeight, Math.min(590, Screen.height * 0.78))
    minimumWidth: 700
    minimumHeight: 470
    visible: true
    title: qsTr("Fidan Files")
    x: Math.max(78, (Screen.width - width) / 2)
    y: Math.max(60, (Screen.height - height) / 2)
    color: "transparent"
    flags: Qt.Window | Qt.FramelessWindowHint

    readonly property color accent: UiStyle.color("#63DE8A", UiStyle.dark)
    readonly property color foreground: UiStyle.color("#F2F7F4", UiStyle.dark)
    readonly property color secondary: UiStyle.color("#9FB2A8", UiStyle.dark)
    readonly property bool maximized: visibility === Window.Maximized
    property int menuIndex: -1
    property var selectedRows: []
    property int selectionAnchor: -1
    property var itemProperties: ({})
    function selection() { return selectedRows.length ? selectedRows : grid.currentIndex >= 0 ? [grid.currentIndex] : [] }
    function selectRow(row, modifiers) {
        let rows = selectedRows.slice()
        if ((modifiers & Qt.ShiftModifier) && selectionAnchor >= 0) {
            rows = []
            for (let i = Math.min(row, selectionAnchor); i <= Math.max(row, selectionAnchor); ++i) rows.push(i)
        } else if (modifiers & Qt.ControlModifier) {
            const at = rows.indexOf(row)
            if (at >= 0) rows.splice(at, 1); else rows.push(row)
            selectionAnchor = row
        } else { rows = [row]; selectionAnchor = row }
        selectedRows = rows; grid.currentIndex = row
    }
    Connections { target: filesBackend; function onModelReset() { root.selectedRows = []; root.selectionAnchor = -1 } }
    Shortcut { sequence: "Ctrl+A"; enabled: grid.activeFocus; onActivated: { let rows = []; for (let i = 0; i < grid.count; ++i) rows.push(i); root.selectedRows = rows } }
    Shortcut { sequence: "Ctrl+C"; enabled: grid.activeFocus; onActivated: filesBackend.copyEntries(root.selection(), false) }
    Shortcut { sequence: "Ctrl+X"; enabled: grid.activeFocus; onActivated: filesBackend.copyEntries(root.selection(), true) }
    Shortcut { sequence: "Ctrl+V"; enabled: grid.activeFocus && !filesBackend.busy; onActivated: filesBackend.paste() }
    Shortcut { sequence: "Ctrl+H"; onActivated: filesBackend.showHidden = !filesBackend.showHidden }
    Shortcut { sequence: "Alt+Return"; enabled: grid.activeFocus; onActivated: root.openProperties(grid.currentIndex) }
    function openProperties(row) { if (row >= 0) { itemProperties = filesBackend.properties(row); propertiesDialog.open() } }
    onClosing: function(close) { if (filesBackend.busy) { close.accepted = false; operationDialog.open() } }


    function toggleMaximized() { maximized ? showNormal() : showMaximized() }
    function createEntry() {
        const ok = createDialog.kind === "folder"
            ? filesBackend.createFolder(entryName.text) : filesBackend.createFile(entryName.text)
        if (ok) createDialog.close()
    }
    function renameEntry() {
        if (filesBackend.renameEntry(root.menuIndex, renameName.text)) renameDialog.close()
    }
    function startCreating(kind) {
        if (filesBackend.inTrash || filesBackend.busy) return
        createDialog.kind = kind
        createDialog.open()
    }
    Shortcut { sequence: "Ctrl+L"; onActivated: { pathField.forceActiveFocus(); pathField.selectAll() } }
    Shortcut { sequence: "Ctrl+F"; onActivated: search.forceActiveFocus() }
    Shortcut { sequence: "Ctrl+N"; onActivated: root.startCreating("file") }
    Shortcut { sequence: "Ctrl+Shift+N"; onActivated: root.startCreating("folder") }
    Shortcut { sequence: "Alt+Left"; onActivated: filesBackend.back() }
    Shortcut { sequence: "Alt+Right"; onActivated: filesBackend.forward() }
    Shortcut { sequence: "Alt+Up"; onActivated: filesBackend.up() }
    Shortcut { sequence: "F5"; onActivated: filesBackend.refresh() }
    function openRename(row) {
        if (row < 0 || filesBackend.inTrash || filesBackend.busy) return
        menuIndex = row
        renameName.text = filesBackend.entryName(row)
        renameDialog.open()
        renameName.forceActiveFocus()
        renameName.selectAll()
    }
    function openDelete(row) {
        if (row < 0) return
        menuIndex = row
        deleteDialog.open()
    }

    component WinButton: Rectangle {
        id: button
        property string symbol
        property bool danger: false
        signal invoked
        width: 46; height: 42
        color: mouse.containsMouse ? (danger ? UiStyle.color("#C74646", UiStyle.dark) : UiStyle.color("#304039", UiStyle.dark)) : "transparent"
        Text { anchors.centerIn: parent; text: button.symbol; color: button.danger && mouse.containsMouse ? "white" : root.foreground; font.pixelSize: 14 }
        MouseArea { id: mouse; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: button.invoked() }
    }

    component ToolButton: Rectangle {
        id: button
        property string symbol
        property string tip: ""
        property string iconName: ""
        property bool enabled: true
        signal invoked
        width: 34; height: 32; radius: 7
        opacity: enabled ? 1 : 0.4
        color: mouse.containsMouse ? UiStyle.color("#315F46", UiStyle.dark) : UiStyle.color("#1A2823", UiStyle.dark)
        Text { anchors.centerIn: parent; visible: button.iconName.length === 0; text: button.symbol; color: root.foreground; font.pixelSize: 16 }
        Image { anchors.centerIn: parent; visible: button.iconName.length > 0; width: 18; height: 18; source: button.iconName.length ? "image://theme/" + button.iconName + (UiStyle.dark ? "?theme=dark" : "?theme=light") : ""; sourceSize: Qt.size(22, 22) }
        MouseArea { id: mouse; anchors.fill: parent; enabled: button.enabled; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: button.invoked() }
        ToolTip.visible: mouse.containsMouse
        ToolTip.text: button.tip
    }

    component ActionButton: Rectangle {
        id: button
        property string label
        property bool primary: false
        property bool danger: false
        signal invoked
        implicitWidth: labelItem.implicitWidth + 28
        implicitHeight: 34
        radius: 7
        color: danger ? (mouse.containsMouse ? UiStyle.color("#B94343", UiStyle.dark) : UiStyle.color("#8F3434", UiStyle.dark))
                      : primary ? (mouse.containsMouse ? UiStyle.color("#397D58", UiStyle.dark) : UiStyle.color("#28533E", UiStyle.dark))
                                : (mouse.containsMouse ? UiStyle.color("#304039", UiStyle.dark) : UiStyle.color("#1A2823", UiStyle.dark))
        border.width: 1
        border.color: danger ? UiStyle.color("#D25A5A", UiStyle.dark) : primary ? UiStyle.color("#347551", UiStyle.dark) : UiStyle.color("#465750", UiStyle.dark)
        Text { id: labelItem; anchors.centerIn: parent; text: button.label; color: button.danger ? "white" : root.foreground; font.family: "Inter"; font.pixelSize: 12; font.weight: Font.DemiBold }
        MouseArea { id: mouse; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: button.invoked() }
    }

    component DarkMenuItem: MenuItem {
        id: item
        height: 38
        leftPadding: 13
        rightPadding: 13
        contentItem: Text { text: item.text; color: item.enabled ? root.foreground : UiStyle.color("#68776F", UiStyle.dark); font.family: "Inter"; font.pixelSize: 12; verticalAlignment: Text.AlignVCenter }
        background: Rectangle { radius: 6; color: item.highlighted ? UiStyle.color("#28533E", UiStyle.dark) : "transparent" }
    }

    background: Rectangle {
        radius: root.maximized ? 0 : 13
        color: UiStyle.color("#0E1715", UiStyle.dark)
        border.width: root.maximized ? 0 : 1
        border.color: UiStyle.color("#465750", UiStyle.dark)
        clip: true
    }

    Rectangle {
        id: titleBar
        anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
        height: 42; color: UiStyle.color("#101A17", UiStyle.dark)
        MouseArea {
            anchors.left: parent.left; anchors.right: controls.left; anchors.top: parent.top; anchors.bottom: parent.bottom
            cursorShape: root.maximized ? Qt.ArrowCursor : Qt.SizeAllCursor
            onPressed: root.startSystemMove()
            onDoubleClicked: root.toggleMaximized()
        }
        Row {
            anchors.left: parent.left; anchors.leftMargin: 14; anchors.verticalCenter: parent.verticalCenter; spacing: 9
            Image { width: 18; height: 18; source: "image://theme/system-file-manager" + (UiStyle.dark ? "?theme=dark" : "?theme=light"); sourceSize: Qt.size(22, 22) }
            Text { text: qsTr("Files"); color: root.foreground; font.family: "Inter"; font.pixelSize: 12; font.weight: Font.DemiBold }
        }
        Row {
            id: controls
            anchors.right: parent.right
            WinButton { symbol: "−"; onInvoked: root.showMinimized() }
            WinButton { symbol: root.maximized ? "❐" : "□"; onInvoked: root.toggleMaximized() }
            WinButton { symbol: "×"; danger: true; onInvoked: root.close() }
        }
    }

    Rectangle {
        id: toolbar
        anchors.left: parent.left; anchors.right: parent.right; anchors.top: titleBar.bottom
        height: 52; color: UiStyle.color("#111C19", UiStyle.dark); border.width: 1; border.color: UiStyle.color("#2B3A35", UiStyle.dark)
        RowLayout {
            anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 12; spacing: 7
            ToolButton { symbol: "‹"; tip: qsTr("Back"); enabled: filesBackend.canGoBack; onInvoked: filesBackend.back() }
            ToolButton { symbol: "›"; tip: qsTr("Forward"); enabled: filesBackend.canGoForward; onInvoked: filesBackend.forward() }
            ToolButton { symbol: "↑"; tip: qsTr("Up"); onInvoked: filesBackend.up() }
            ToolButton { symbol: "⌂"; tip: qsTr("Home"); onInvoked: filesBackend.home() }
            Rectangle {
                Layout.fillWidth: true; Layout.preferredHeight: 34; radius: 7; color: UiStyle.color("#17221F", UiStyle.dark)
                border.width: 1; border.color: pathField.activeFocus ? root.accent : UiStyle.color("#34473F", UiStyle.dark)
                TextField {
                    id: pathField
                    objectName: "folderPath"
                    anchors.fill: parent; leftPadding: 11; rightPadding: 11
                    text: filesBackend.path; color: root.foreground; selectByMouse: true
                    font.family: "Inter"; font.pixelSize: 12; background: null
                    onAccepted: { filesBackend.setPath(text); text = filesBackend.path; grid.forceActiveFocus() }
                    Connections { target: filesBackend; function onPathChanged() { pathField.text = filesBackend.path } }
                }
            }
            ToolButton { symbol: "+"; enabled: !filesBackend.inTrash && !filesBackend.busy; tip: qsTr("New"); onInvoked: createMenu.open() }
            ToolButton { iconName: "edit-copy"; tip: qsTr("Copy (Ctrl+C)"); enabled: !filesBackend.busy && root.selection().length > 0 && !filesBackend.inTrash; onInvoked: filesBackend.copyEntries(root.selection(), false) }
            ToolButton { iconName: "edit-paste"; tip: qsTr("Paste (Ctrl+V)"); enabled: !filesBackend.busy && filesBackend.canPaste; onInvoked: filesBackend.paste() }
            ToolButton { symbol: "⋯"; tip: qsTr("View and sort"); onInvoked: viewMenu.open() }
            ToolButton { symbol: "↻"; tip: qsTr("Refresh"); onInvoked: filesBackend.refresh() }
        }
    }

    Rectangle {
        id: sidebar
        anchors.left: parent.left; anchors.top: toolbar.bottom; anchors.bottom: status.top
        width: 178; color: UiStyle.color("#111C19", UiStyle.dark)
        Column {
            anchors.fill: parent; anchors.margins: 10; spacing: 5
            Text { leftPadding: 9; bottomPadding: 5; text: qsTr("Places"); color: root.secondary; font.family: "Inter"; font.pixelSize: 11; font.weight: Font.DemiBold }
            Repeater {
                model: [
                    { n: qsTr("Home"), i: "go-home", p: "home" },
                    { n: qsTr("Desktop"), i: "user-desktop", p: "desktop" },
                    { n: qsTr("Documents"), i: "folder-documents", p: "documents" },
                    { n: qsTr("Downloads"), i: "folder-download", p: "downloads" },
                    { n: qsTr("Pictures"), i: "folder-pictures", p: "pictures" },
                    { n: qsTr("Videos"), i: "folder-videos", p: "videos" },
                    { n: qsTr("Music"), i: "folder-music", p: "music" },
                    { n: qsTr("Projects"), i: "folder-development", p: "projects" },
                    { n: qsTr("Trash"), i: "user-trash", p: "trash" }
                ]
                Rectangle {
                    required property var modelData
                    width: parent.width; height: 36; radius: 7
                    color: filesBackend.path === filesBackend.placePath(modelData.p) ? UiStyle.color("#315F46", UiStyle.dark) : (placeMouse.containsMouse ? UiStyle.color("#23332D", UiStyle.dark) : "transparent")
                    Row {
                        anchors.left: parent.left; anchors.leftMargin: 9; anchors.verticalCenter: parent.verticalCenter; spacing: 9
                        Image { width: 17; height: 17; source: "image://theme/" + encodeURIComponent(modelData.i) + (UiStyle.dark ? "?theme=dark" : "?theme=light"); sourceSize: Qt.size(20, 20) }
                        Text { text: modelData.n; color: root.foreground; font.family: "Inter"; font.pixelSize: 10 }
                    }
                    MouseArea {
                        id: placeMouse; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                        onClicked: filesBackend.openPlace(modelData.p)
                    }
                }
            }
        }
    }

    Rectangle {
        id: browser
        anchors.left: sidebar.right; anchors.right: parent.right; anchors.top: toolbar.bottom; anchors.bottom: status.top
        color: UiStyle.color("#0B1412", UiStyle.dark)
        ColumnLayout {
            anchors.fill: parent; anchors.margins: 14; spacing: 10
            RowLayout {
                Layout.fillWidth: true
                Text { text: filesBackend.path.split("/").pop() || "/"; color: root.foreground; font.family: "Inter"; font.pixelSize: 18; font.weight: Font.Bold }
                Item { Layout.fillWidth: true }
                TextField {
                    id: search
                    objectName: "folderSearch"
                    Layout.preferredWidth: 220; Layout.preferredHeight: 34
                    placeholderText: qsTr("Search this folder…"); color: root.foreground; placeholderTextColor: UiStyle.color("#71847A", UiStyle.dark)
                    font.family: "Inter"; font.pixelSize: 12
                    onTextChanged: filesBackend.filter = text
                    onAccepted: grid.forceActiveFocus()
                    Connections { target: filesBackend; function onFilterChanged() { search.text = filesBackend.filter } }
                    background: Rectangle { radius: 7; color: UiStyle.color("#17221F", UiStyle.dark); border.width: search.activeFocus ? 1 : 0; border.color: root.accent }
                }
            }
            Text { Layout.fillWidth: true; visible: filesBackend.errorText.length > 0; text: filesBackend.errorText; wrapMode: Text.Wrap; color: UiStyle.color("#FF9292", UiStyle.dark); font.family: "Inter"; font.pixelSize: 10 }
            RowLayout {
                visible: filesBackend.inTrash; Layout.fillWidth: true
                Text { Layout.fillWidth: true; text: qsTr("Restore selected items to their original location."); color: root.secondary; font.pixelSize: 12 }
                ActionButton { label: qsTr("Restore selected"); onInvoked: filesBackend.restoreEntries(root.selection()) }
            }
            GridView {
                id: grid
                objectName: "fileGrid"
                Layout.fillWidth: true; Layout.fillHeight: true
                clip: true; cellWidth: Math.floor(width / (width >= 480 ? 4 : 3)); cellHeight: 116
                model: filesBackend; boundsBehavior: Flickable.StopAtBounds
                focus: true
                ScrollBar.vertical: ScrollBar { }
                Keys.onPressed: function(event) {
                    if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                        filesBackend.activate(currentIndex)
                        event.accepted = true
                    } else if (event.key === Qt.Key_Delete) {
                        if ((event.modifiers & Qt.ShiftModifier) || filesBackend.inTrash) root.openDelete(currentIndex)
                        else filesBackend.trashEntries(root.selection())
                        event.accepted = true
                    } else if (event.key === Qt.Key_F2) {
                        root.openRename(currentIndex)
                        event.accepted = true
                    } else if ([Qt.Key_Left, Qt.Key_Right, Qt.Key_Up, Qt.Key_Down].indexOf(event.key) >= 0 && count > 0) {
                        const columns = Math.max(1, Math.floor(width / cellWidth))
                        const delta = event.key === Qt.Key_Left ? -1 : event.key === Qt.Key_Right ? 1 : event.key === Qt.Key_Up ? -columns : columns
                        root.selectRow(Math.max(0, Math.min(count - 1, currentIndex + delta)), event.modifiers)
                        positionViewAtIndex(currentIndex, GridView.Contain)
                        event.accepted = true
                    } else if (event.key === Qt.Key_Escape) {
                        root.selectedRows = []; currentIndex = -1; event.accepted = true
                    }
                }
                delegate: Rectangle {
                    id: fileItem
                    required property int index
                    required property string name
                    required property string filePath
                    required property bool directory
                    required property string sizeLabel
                    required property string iconName
                    width: grid.cellWidth - 14; height: 100; radius: 7
                    color: root.selectedRows.indexOf(index) >= 0 || GridView.isCurrentItem ? UiStyle.color("#263F30", UiStyle.dark) : (itemMouse.containsMouse ? UiStyle.color("#23332D", UiStyle.dark) : UiStyle.color("#1B2421", UiStyle.dark))
                    border.width: root.selectedRows.indexOf(index) >= 0 || GridView.isCurrentItem ? 1 : 0; border.color: root.accent
                    DropArea {
                        anchors.fill: parent; enabled: fileItem.directory && !filesBackend.busy && !filesBackend.inTrash
                        onEntered: function(drag) { drag.accepted = drag.hasUrls }
                        onDropped: function(drop) { if (drop.hasUrls) { filesBackend.dropUrls(drop.urls, fileItem.filePath, drop.proposedAction === Qt.MoveAction); drop.acceptProposedAction() } }
                    }
                    Item {
                        id: dragProxy; width: 32; height: 32
                        Drag.active: itemMouse.drag.active && !filesBackend.inTrash && !filesBackend.busy
                        Drag.dragType: Drag.Automatic
                        Drag.supportedActions: Qt.CopyAction | Qt.MoveAction
                        Drag.proposedAction: Qt.CopyAction
                        Drag.mimeData: ({"text/uri-list": filesBackend.entryUrls(root.selection()).join("\r\n")})
                    }
                    Column {
                        anchors.centerIn: parent; width: parent.width - 10; spacing: 7
                        Image { anchors.horizontalCenter: parent.horizontalCenter; width: 48; height: 48; source: "image://theme/" + encodeURIComponent(fileItem.iconName) + (UiStyle.dark ? "?theme=dark" : "?theme=light"); sourceSize: Qt.size(56, 56); fillMode: Image.PreserveAspectFit }
                        Text { width: parent.width; text: fileItem.name; color: root.foreground; elide: Text.ElideRight; horizontalAlignment: Text.AlignHCenter; font.family: "Inter"; font.pixelSize: 12; font.weight: Font.DemiBold }
                        Text { visible: fileItem.sizeLabel.length > 0; width: parent.width; text: fileItem.sizeLabel; color: root.secondary; horizontalAlignment: Text.AlignHCenter; font.family: "Inter"; font.pixelSize: 10 }
                    }
                    MouseArea {
                        id: itemMouse
                        anchors.fill: parent; hoverEnabled: true
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        cursorShape: Qt.PointingHandCursor
                        onClicked: function(mouse) {
                            if (mouse.button !== Qt.RightButton || root.selectedRows.indexOf(fileItem.index) < 0) root.selectRow(fileItem.index, mouse.modifiers)
                            grid.forceActiveFocus()
                            if (mouse.button === Qt.RightButton) {
                                root.menuIndex = fileItem.index
                                const point = itemMouse.mapToItem(root.contentItem, mouse.x, mouse.y)
                                itemMenu.x = point.x
                                itemMenu.y = point.y
                                itemMenu.open()
                            }
                        }
                        drag.target: dragProxy
                        onPressed: function(mouse) {
                            if (mouse.button === Qt.LeftButton && !(mouse.modifiers & (Qt.ControlModifier | Qt.ShiftModifier)) && root.selectedRows.indexOf(fileItem.index) < 0) root.selectRow(fileItem.index, mouse.modifiers)
                        }
                        onDoubleClicked: function(mouse) { if (mouse.button === Qt.LeftButton) filesBackend.activate(fileItem.index) }
                    }
                }
                Text { anchors.centerIn: parent; visible: grid.count === 0; text: filesBackend.filter.length > 0 ? qsTr("No files match your search") : qsTr("This folder is empty"); color: root.secondary; font.family: "Inter" }
            }
        }
        DropArea {
            anchors.fill: parent; z: -0.5; enabled: !filesBackend.busy && !filesBackend.inTrash
            onEntered: function(drag) { drag.accepted = drag.hasUrls }
            onDropped: function(drop) { if (drop.hasUrls) { filesBackend.dropUrls(drop.urls, "", drop.proposedAction === Qt.MoveAction); drop.acceptProposedAction() } }
        }
        MouseArea {
            anchors.fill: parent; acceptedButtons: Qt.RightButton; z: -1
            onClicked: function(mouse) {
                const point = mapToItem(root.contentItem, mouse.x, mouse.y)
                folderMenu.x = point.x; folderMenu.y = point.y; folderMenu.open()
            }
        }
    }

    Rectangle {
        id: status
        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
        height: 34; color: UiStyle.color("#101A17", UiStyle.dark); border.width: 1; border.color: UiStyle.color("#2B3A35", UiStyle.dark)
        Text { anchors.left: parent.left; anchors.leftMargin: 190; anchors.verticalCenter: parent.verticalCenter; text: filesBackend.busy ? filesBackend.operationStatus : qsTr("%1 items · %2 selected").arg(grid.count).arg(root.selectedRows.length); color: root.secondary; font.family: "Inter"; font.pixelSize: 11 }
        Text { anchors.right: parent.right; anchors.rightMargin: 14; anchors.verticalCenter: parent.verticalCenter; text: grid.currentItem ? grid.currentItem.name : ""; color: root.secondary; font.family: "Inter"; font.pixelSize: 11 }
    }

    Menu {
        id: createMenu
        y: toolbar.y + toolbar.height
        x: root.width - 190
        width: 170
        background: Rectangle { radius: 9; color: UiStyle.color("#15211D", UiStyle.dark); border.width: 1; border.color: UiStyle.color("#465750", UiStyle.dark) }
        DarkMenuItem { text: qsTr("New Folder"); onTriggered: { createDialog.kind = "folder"; createDialog.open() } }
        DarkMenuItem { text: qsTr("New File"); onTriggered: { createDialog.kind = "file"; createDialog.open() } }
    }

    Menu {
        id: itemMenu
        width: 190
        background: Rectangle { radius: 9; color: UiStyle.color("#15211D", UiStyle.dark); border.width: 1; border.color: UiStyle.color("#465750", UiStyle.dark) }
        DarkMenuItem { text: qsTr("Open"); onTriggered: filesBackend.activate(root.menuIndex) }
        DarkMenuItem { text: qsTr("Copy"); enabled: !filesBackend.inTrash && !filesBackend.busy; onTriggered: filesBackend.copyEntries(root.selection(), false) }
        DarkMenuItem { text: qsTr("Cut"); enabled: !filesBackend.inTrash && !filesBackend.busy; onTriggered: filesBackend.copyEntries(root.selection(), true) }
        DarkMenuItem { text: qsTr("Restore"); visible: filesBackend.inTrash; enabled: !filesBackend.busy; onTriggered: filesBackend.restoreEntries(root.selection()) }
        DarkMenuItem { text: qsTr("Rename"); enabled: !filesBackend.inTrash && !filesBackend.busy; onTriggered: root.openRename(root.menuIndex) }
        MenuSeparator { contentItem: Rectangle { implicitHeight: 1; color: UiStyle.color("#34473F", UiStyle.dark) } }
        DarkMenuItem { text: qsTr("Move to Trash"); enabled: !filesBackend.inTrash && !filesBackend.busy; onTriggered: filesBackend.trashEntries(root.selection()) }
        DarkMenuItem { text: qsTr("Delete Permanently…"); enabled: !filesBackend.busy; onTriggered: root.openDelete(root.menuIndex) }
        DarkMenuItem { text: qsTr("Properties…"); onTriggered: root.openProperties(root.menuIndex) }
    }

    Menu {
        id: folderMenu
        width: 190
        background: Rectangle { radius: 9; color: UiStyle.color("#15211D", UiStyle.dark); border.width: 1; border.color: UiStyle.color("#465750", UiStyle.dark) }
        DarkMenuItem { text: qsTr("New Folder"); onTriggered: { createDialog.kind = "folder"; createDialog.open() } }
        DarkMenuItem { text: qsTr("New File"); onTriggered: { createDialog.kind = "file"; createDialog.open() } }
        MenuSeparator { contentItem: Rectangle { implicitHeight: 1; color: UiStyle.color("#34473F", UiStyle.dark) } }
        DarkMenuItem { text: qsTr("Paste"); enabled: filesBackend.canPaste && !filesBackend.busy; onTriggered: filesBackend.paste() }
        DarkMenuItem { text: qsTr("Open in Terminal"); onTriggered: filesBackend.openTerminalHere() }
        DarkMenuItem { text: qsTr("Refresh"); onTriggered: filesBackend.refresh() }
    }

    Dialog {
        id: createDialog
        objectName: "createDialog"
        property string kind: "folder"
        anchors.centerIn: parent; width: 360; modal: true
        title: kind === "folder" ? qsTr("New Folder") : qsTr("New File")
        header: null
        standardButtons: Dialog.NoButton
        onOpened: { entryName.clear(); entryName.forceActiveFocus() }
        background: Rectangle { radius: 11; color: UiStyle.color("#17231F", UiStyle.dark); border.width: 1; border.color: UiStyle.color("#465750", UiStyle.dark) }
        contentItem: ColumnLayout {
            spacing: 12
            Text { text: createDialog.title; color: root.foreground; font.family: "Inter"; font.pixelSize: 14; font.weight: Font.Bold }
            TextField { id: entryName; objectName: "newEntryName"; Layout.fillWidth: true; placeholderText: createDialog.kind === "folder" ? qsTr("Folder name") : qsTr("File name"); color: root.foreground; selectByMouse: true; onAccepted: root.createEntry(); background: Rectangle { radius: 7; color: UiStyle.color("#0E1715", UiStyle.dark); border.width: 1; border.color: root.accent } }
            Text { Layout.fillWidth: true; visible: filesBackend.errorText.length > 0; text: filesBackend.errorText; wrapMode: Text.Wrap; color: UiStyle.color("#FF9292", UiStyle.dark); font.pixelSize: 12 }
            RowLayout {
                Item { Layout.fillWidth: true }
                ActionButton { label: qsTr("Cancel"); onInvoked: createDialog.close() }
                ActionButton { label: qsTr("Create"); primary: true; onInvoked: root.createEntry() }
            }
        }
    }

    Dialog {
        id: renameDialog
        anchors.centerIn: parent; width: 360; modal: true
        standardButtons: Dialog.NoButton
        background: Rectangle { radius: 11; color: UiStyle.color("#17231F", UiStyle.dark); border.width: 1; border.color: UiStyle.color("#465750", UiStyle.dark) }
        contentItem: ColumnLayout {
            spacing: 12
            Text { text: qsTr("Rename"); color: root.foreground; font.family: "Inter"; font.pixelSize: 14; font.weight: Font.Bold }
            TextField { id: renameName; objectName: "renameEntryName"; Layout.fillWidth: true; color: root.foreground; selectByMouse: true; onAccepted: root.renameEntry(); background: Rectangle { radius: 7; color: UiStyle.color("#0E1715", UiStyle.dark); border.width: 1; border.color: root.accent } }
            Text { Layout.fillWidth: true; visible: filesBackend.errorText.length > 0; text: filesBackend.errorText; wrapMode: Text.Wrap; color: UiStyle.color("#FF9292", UiStyle.dark); font.pixelSize: 12 }
            RowLayout {
                Item { Layout.fillWidth: true }
                ActionButton { label: qsTr("Cancel"); onInvoked: renameDialog.close() }
                ActionButton { label: qsTr("Rename"); primary: true; onInvoked: root.renameEntry() }
            }
        }
    }

    Dialog {
        id: deleteDialog
        anchors.centerIn: parent; width: 390; modal: true
        standardButtons: Dialog.NoButton
        background: Rectangle { radius: 11; color: UiStyle.color("#17231F", UiStyle.dark); border.width: 1; border.color: UiStyle.color("#6D4545", UiStyle.dark) }
        contentItem: ColumnLayout {
            spacing: 12
            Text { text: qsTr("Delete permanently?"); color: root.foreground; font.family: "Inter"; font.pixelSize: 14; font.weight: Font.Bold }
            Text { Layout.fillWidth: true; text: qsTr("%1 selected items will be deleted and cannot be recovered.").arg(root.selection().length); color: root.secondary; wrapMode: Text.Wrap; font.family: "Inter"; font.pixelSize: 10 }
            RowLayout {
                Item { Layout.fillWidth: true }
                ActionButton { label: qsTr("Cancel"); onInvoked: deleteDialog.close() }
                ActionButton { label: qsTr("Delete"); danger: true; onInvoked: { filesBackend.trashEntries(root.selection(), true); deleteDialog.close() } }
            }
        }
    }

    MouseArea { anchors.left: parent.left; anchors.top: titleBar.bottom; anchors.bottom: parent.bottom; width: 6; visible: !root.maximized; cursorShape: Qt.SizeHorCursor; onPressed: root.startSystemResize(Qt.LeftEdge) }
    MouseArea { anchors.right: parent.right; anchors.top: titleBar.bottom; anchors.bottom: parent.bottom; width: 6; visible: !root.maximized; cursorShape: Qt.SizeHorCursor; onPressed: root.startSystemResize(Qt.RightEdge) }
    MouseArea { anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; height: 6; visible: !root.maximized; cursorShape: Qt.SizeVerCursor; onPressed: root.startSystemResize(Qt.BottomEdge) }

    Menu {
        id: viewMenu; x: root.width - width - 12; y: toolbar.y + toolbar.height; width: 230
        background: Rectangle { radius: 9; color: UiStyle.color("#15211D", UiStyle.dark); border.width: 1; border.color: UiStyle.color("#465750", UiStyle.dark) }
        DarkMenuItem { text: qsTr("Sort: Name"); onTriggered: filesBackend.sortBy = "name" }
        DarkMenuItem { text: qsTr("Sort: Size"); onTriggered: filesBackend.sortBy = "size" }
        DarkMenuItem { text: qsTr("Sort: Modified"); onTriggered: filesBackend.sortBy = "modified" }
        DarkMenuItem { text: filesBackend.showHidden ? qsTr("Hide hidden files (Ctrl+H)") : qsTr("Show hidden files (Ctrl+H)"); onTriggered: filesBackend.showHidden = !filesBackend.showHidden }
    }
    Dialog {
        id: propertiesDialog; objectName: "propertiesDialog"; anchors.centerIn: parent; width: Math.min(480, root.width - 32); modal: true; header: null
        background: Rectangle { radius: 11; color: UiStyle.color("#17231F", UiStyle.dark); border.width: 1; border.color: UiStyle.color("#465750", UiStyle.dark) }
        contentItem: ColumnLayout {
            spacing: 12
            Text { text: qsTr("Properties"); color: root.foreground; font.pixelSize: 16; font.bold: true }
            Text { Layout.fillWidth: true; text: root.itemProperties.name || ""; wrapMode: Text.Wrap; color: root.foreground; font.bold: true }
            TextEdit { Layout.fillWidth: true; text: root.itemProperties.path || ""; readOnly: true; selectByMouse: true; wrapMode: TextEdit.Wrap; color: root.secondary; font.pixelSize: 12 }
            Text { Layout.fillWidth: true; wrapMode: Text.Wrap; color: root.secondary; font.pixelSize: 12; text: (root.itemProperties.type || "") + " · " + (root.itemProperties.size || "") + "\n" + qsTr("Modified: %1").arg(root.itemProperties.modified || "") + "\n" + qsTr("Owner: %1").arg(root.itemProperties.owner || "") + "\n" + qsTr("Readable: %1 · Writable: %2").arg(root.itemProperties.readable ? qsTr("Yes") : qsTr("No")).arg(root.itemProperties.writable ? qsTr("Yes") : qsTr("No")) + (root.itemProperties.linkTarget ? "\n" + root.itemProperties.linkTarget : "") }
            ActionButton { Layout.alignment: Qt.AlignRight; label: qsTr("Close"); onInvoked: propertiesDialog.close() }
        }
    }
    Dialog {
        id: operationDialog; anchors.centerIn: parent; modal: true; header: null; width: 340
        background: Rectangle { radius: 11; color: UiStyle.color("#17231F", UiStyle.dark) }
        contentItem: ColumnLayout {
            Text { Layout.fillWidth: true; text: qsTr("Wait for the file operation to finish before closing this window."); wrapMode: Text.Wrap; color: root.foreground }
            ActionButton { label: qsTr("OK"); onInvoked: operationDialog.close() }
        }
    }
}
