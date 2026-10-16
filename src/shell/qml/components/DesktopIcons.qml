// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import Fidan.Shell

FocusScope {
    id: root
    readonly property real interfaceScale: Controller.previewMode ? 1 : Controller.interfaceScale
    property var selectedPaths: []
    property var allPaths: []
    property string selectionAnchor: ""
    property var draggedUrls: []
    property string draggedIcon: "application-x-executable"
    property bool dragInProgress: false
    readonly property int selectedCount: selectedPaths.length
    visible: Controller.desktopIconsVisible
    clip: true

    function reloadPaths() {
        allPaths = DesktopItems.paths()
        selectedPaths = selectedPaths.filter(path => allPaths.indexOf(path) >= 0)
        if (allPaths.indexOf(selectionAnchor) < 0) selectionAnchor = ""
    }
    function clearSelection() { selectedPaths = []; selectionAnchor = "" }
    function select(path, modifiers) {
        let paths = selectedPaths.slice()
        if ((modifiers & Qt.ShiftModifier) && selectionAnchor.length) {
            const first = allPaths.indexOf(selectionAnchor), last = allPaths.indexOf(path)
            const range = allPaths.slice(Math.min(first,last), Math.max(first,last)+1)
            paths = modifiers & Qt.ControlModifier ? paths.concat(range) : range
        } else if (modifiers & Qt.ControlModifier) {
            const position = paths.indexOf(path)
            if (position < 0) paths.push(path); else paths.splice(position,1)
            selectionAnchor = path
        } else { paths = [path]; selectionAnchor = path }
        selectedPaths = paths.filter((value,index,array) => array.indexOf(value) === index)
    }
    function prepareDrag() { draggedUrls = DesktopItems.dragUrls(selectedPaths) }
    function finishDrag() { dragInProgress = false }
    function updateBox() {
        if (!selectionMouse.selecting) return
        let paths = selectionMouse.baseSelection.slice()
        for (let i=0; i<grid.count; ++i) {
            const item = grid.itemAtIndex(i)
            if (!item) continue
            const corner = item.mapToItem(root,0,0)
            const end = item.mapToItem(root,item.width,item.height)
            if (corner.x < selectionBox.x+selectionBox.width && end.x > selectionBox.x
                && corner.y < selectionBox.y+selectionBox.height && end.y > selectionBox.y)
                paths.push(item.path)
        }
        selectedPaths = paths.filter((value,index,array) => array.indexOf(value) === index)
    }
    Component.onCompleted: reloadPaths()
    Connections { target: DesktopItems; function onModelReset() { root.reloadPaths() } }
    Connections { target: DesktopFiles; function onBusyChanged() { if (!DesktopFiles.busy) DesktopItems.refresh() } }
    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_A && (event.modifiers & Qt.ControlModifier)) {
            root.selectedPaths = root.allPaths.slice(); event.accepted = true
        } else if (event.key === Qt.Key_Escape) {
            selectionMouse.selecting = false; root.clearSelection(); event.accepted = true
        }
    }

    DropArea {
        anchors.fill: parent
        enabled: !DesktopFiles.busy
        onEntered: function(drag) { drag.accepted = drag.hasUrls && DesktopItems.canDrop(drag.urls,DesktopItems.directoryPath) }
        onDropped: function(drop) {
            if (drop.hasUrls && DesktopItems.canDrop(drop.urls,DesktopItems.directoryPath)) {
                DesktopFiles.dropUrls(drop.urls,DesktopItems.directoryPath,drop.proposedAction === Qt.MoveAction)
                drop.acceptProposedAction()
            }
        }
    }
    MouseArea {
        id: selectionMouse
        objectName: "desktop-selection-area"
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        preventStealing: true
        property point origin
        property point end
        property var baseSelection: []
        property bool selecting: false
        onPressed: function(mouse) {
            root.forceActiveFocus()
            origin = Qt.point(mouse.x,mouse.y); end = origin
            baseSelection = mouse.modifiers & (Qt.ControlModifier | Qt.ShiftModifier) ? root.selectedPaths.slice() : []
            if (!baseSelection.length) root.clearSelection()
        }
        onPositionChanged: function(mouse) {
            if (!pressed) return
            end = Qt.point(Math.max(0,Math.min(root.width,mouse.x)),Math.max(0,Math.min(root.height,mouse.y)))
            if (!selecting && Math.hypot(end.x-origin.x,end.y-origin.y) >= Qt.styleHints.startDragDistance) selecting = true
            root.updateBox()
        }
        onReleased: { root.updateBox(); selecting = false }
        onCanceled: selecting = false
    }
    // The native drag outlives its source delegate when a move refreshes the
    // model. Keep its MIME data and owner alive until QDrag actually finishes.
    Item {
        id: dragProxy
        objectName: "desktop-drag-proxy"
        width: 44; height: 44
        Drag.dragType: Drag.None
        Drag.supportedActions: Qt.CopyAction | Qt.MoveAction
        Drag.proposedAction: Qt.MoveAction
        Drag.mimeData: ({"text/uri-list": root.draggedUrls.join("\r\n")})
        Drag.imageSource: "image://theme/" + encodeURIComponent(root.draggedIcon)
        Drag.hotSpot.x: 22; Drag.hotSpot.y: 22
        Drag.onDragFinished: { dragProxy.Drag.active = false; root.finishDrag(); x = 0; y = 0 }
    }
    GridView {
        id: grid
        objectName: "desktop-grid"
        anchors.fill: parent
        cellWidth: 96 * root.interfaceScale
        cellHeight: 96 * root.interfaceScale
        model: DesktopItems
        flow: GridView.FlowTopToBottom
        interactive: false
        boundsBehavior: Flickable.StopAtBounds
        currentIndex: -1
        delegate: Rectangle {
            id: icon
            required property int index
            required property string name
            required property string iconName
            required property string path
            required property bool isDirectory
            property bool wasSelected: false
            property bool wasDragged: false
            objectName: "desktop-icon-" + index
            readonly property bool selected: root.selectedPaths.indexOf(path) >= 0
            width: 88; height: 88
            scale: root.interfaceScale
            transformOrigin: Item.TopLeft
            radius: 9
            color: folderDrop.containsDrag ? "#A0315F46" : selected ? "#80315F46" : iconMouse.containsMouse ? "#602B3A34" : "transparent"
            border.width: selected || folderDrop.containsDrag ? 1 : 0
            border.color: Theme.accent
            DropArea {
                id: folderDrop
                objectName: "desktop-folder-drop-" + icon.index
                anchors.fill: parent
                enabled: icon.isDirectory && !DesktopFiles.busy
                onEntered: function(drag) { drag.accepted = drag.hasUrls && DesktopItems.canDrop(drag.urls,icon.path) }
                onDropped: function(drop) {
                    if (drop.hasUrls && DesktopItems.canDrop(drop.urls,icon.path)) {
                        DesktopFiles.dropUrls(drop.urls,icon.path,drop.proposedAction === Qt.MoveAction)
                        drop.acceptProposedAction()
                    }
                }
            }
            Column {
                anchors.centerIn: parent
                width: parent.width - 8
                spacing: 5
                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 44; height: 44; radius: 10; color: "#315F46"
                    SystemIcon { anchors.centerIn: parent; width: 27; height: 27; visible: icon.isDirectory; name: "folder"; color: "white" }
                    Image {
                        id: artwork
                        cache: false
                        anchors.fill: parent; anchors.margins: 5
                        visible: !icon.isDirectory
                        source: "image://theme/" + encodeURIComponent(icon.iconName) + (Controller.darkTheme ? "?theme=dark" : "?theme=light")
                        sourceSize: Qt.size(36,36); fillMode: Image.PreserveAspectFit; asynchronous: true
                    }
                }
                Text {
                    width: parent.width; text: icon.name; textFormat: Text.PlainText; color: "white"
                    font.family: Theme.fontFamily; font.pixelSize: 10; horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight; style: Text.Outline; styleColor: "#A0000000"
                }
            }
            MouseArea {
                id: iconMouse
                objectName: "desktop-icon-mouse-" + icon.index
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                hoverEnabled: true
                preventStealing: true
                cursorShape: Qt.PointingHandCursor
                drag.target: DesktopFiles.busy ? null : dragProxy
                onPressed: function(mouse) {
                    root.forceActiveFocus()
                    icon.wasDragged = false
                    icon.wasSelected = icon.selected
                    if (!icon.wasSelected) root.select(icon.path,mouse.modifiers)
                    root.draggedIcon = icon.isDirectory ? "folder" : icon.iconName
                    root.prepareDrag()
                }
                onPositionChanged: {
                    if (drag.active && !icon.wasDragged && !DesktopFiles.busy) {
                        icon.wasDragged = true
                        root.dragInProgress = true
                        // Native startDrag finishes synchronously. Queue it
                        // outside active-property bindings to avoid reentry.
                        Qt.callLater(function() {
                            if (!root.dragInProgress) return
                            dragProxy.Drag.active = true
                            dragProxy.Drag.startDrag()
                        })
                    }
                }
                onClicked: function(mouse) {
                    if (root.dragInProgress || icon.wasDragged) return
                    if (!(mouse.modifiers & Qt.ControlModifier) || icon.wasSelected) root.select(icon.path,mouse.modifiers)
                }
                onDoubleClicked: {
                    if (!root.dragInProgress && !icon.wasDragged && !DesktopItems.open(icon.index))
                        Notifications.Notify("Files",0,"dialog-warning",qsTr("Could not open item"),icon.name,[],{},7000)
                }
            }
        }
    }
    Rectangle {
        id: selectionBox
        objectName: "desktop-selection-box"
        z: 5
        x: Math.min(selectionMouse.origin.x,selectionMouse.end.x)
        y: Math.min(selectionMouse.origin.y,selectionMouse.end.y)
        width: Math.abs(selectionMouse.end.x-selectionMouse.origin.x)
        height: Math.abs(selectionMouse.end.y-selectionMouse.origin.y)
        visible: selectionMouse.selecting
        color: "#3063DE8A"; border.width: 1; border.color: Theme.accent
    }
}
