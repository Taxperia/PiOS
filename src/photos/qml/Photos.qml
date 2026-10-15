// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Window

ApplicationWindow {
    id: root
    width: Math.max(720, Math.min(1020, Screen.width * 0.82))
    height: Math.max(480, Math.min(680, Screen.height * 0.82))
    minimumWidth: 680; minimumHeight: 440
    visible: true; title: PhotoLibrary.name.length ? PhotoLibrary.name + " — Fidan Photos" : qsTr("Fidan Photos")
    flags: Qt.Window | Qt.FramelessWindowHint; color: "transparent"
    property bool fitMode: true
    property real zoom: 1
    property string message: ""
    readonly property bool maximized: visibility === Window.Maximized || visibility === Window.FullScreen
    readonly property color foreground: UiStyle.color("#F2F7F4", UiStyle.dark)
    readonly property color secondary: UiStyle.color("#9FB2A8", UiStyle.dark)
    readonly property bool turned: PhotoLibrary.rotation === 90 || PhotoLibrary.rotation === 270
    readonly property real imageWidth: turned ? PhotoLibrary.pixelHeight : PhotoLibrary.pixelWidth
    readonly property real imageHeight: turned ? PhotoLibrary.pixelWidth : PhotoLibrary.pixelHeight
    readonly property real actualZoom: fitMode ? Math.max(0.01, Math.min(1, (canvas.width - 24) / Math.max(1, imageWidth), (canvas.height - 24) / Math.max(1, imageHeight))) : zoom
    function toggleMaximized() { maximized ? showNormal() : showMaximized() }
    function zoomBy(factor) {
        const next = Math.max(0.05, Math.min(8, actualZoom * factor))
        fitMode = false; zoom = next
    }
    function fullScreen() { visibility === Window.FullScreen ? showNormal() : showFullScreen() }
    Shortcut { sequence: "Ctrl+O"; onActivated: picker.open() }
    Shortcut { sequence: "Right"; onActivated: PhotoLibrary.next() }
    Shortcut { sequence: "Left"; onActivated: PhotoLibrary.previous() }
    Shortcut { sequence: "+"; onActivated: root.zoomBy(1.25) }
    Shortcut { sequence: "="; onActivated: root.zoomBy(1.25) }
    Shortcut { sequence: "-"; onActivated: root.zoomBy(0.8) }
    Shortcut { sequence: "0"; onActivated: root.fitMode = true }
    Shortcut { sequence: "1"; onActivated: { root.fitMode = false; root.zoom = 1 } }
    Shortcut { sequence: "R"; onActivated: PhotoLibrary.rotate(1) }
    Shortcut { sequence: "Ctrl+C"; onActivated: PhotoLibrary.copyImage() }
    Shortcut { sequence: "F11"; onActivated: root.fullScreen() }
    Shortcut { sequence: "Escape"; enabled: root.visibility === Window.FullScreen; onActivated: root.showNormal() }
    Connections {
        target: PhotoLibrary
        function onCurrentChanged() { root.fitMode = true; root.message = ""; canvas.contentX = 0; canvas.contentY = 0 }
        function onMessage(text) { root.message = text; messageTimer.restart() }
    }
    Timer { id: messageTimer; interval: 4000; onTriggered: root.message = "" }
    FileDialog {
        id: picker; title: qsTr("Open photo")
        nameFilters: [qsTr("Images (*.png *.jpg *.jpeg *.webp *.bmp *.gif *.tif *.tiff *.avif *.heic)"), qsTr("All files (*)")]
        onAccepted: PhotoLibrary.open(selectedFile)
    }
    component Action: Button {
        id: button
        property string iconName: ""
        property string tip: text
        implicitHeight: 34; implicitWidth: iconName.length ? 36 : Math.max(54, label.implicitWidth + 22)
        opacity: enabled ? 1 : 0.35
        contentItem: Item {
            Image { anchors.centerIn: parent; width: 18; height: 18; visible: button.iconName.length > 0; source: visible ? "image://theme/" + button.iconName + (UiStyle.dark ? "?theme=dark" : "?theme=light") : "" }
            Text { id: label; anchors.centerIn: parent; visible: !button.iconName.length; text: button.text; color: root.foreground; font.family: "Inter"; font.pixelSize: 12 }
        }
        background: Rectangle { radius: 7; color: button.hovered ? UiStyle.color("#304039", UiStyle.dark) : UiStyle.color("#1A2823", UiStyle.dark) }
        ToolTip.visible: hovered; ToolTip.text: tip; ToolTip.delay: 500
    }
    background: Rectangle { radius: root.maximized ? 0 : 12; color: UiStyle.color("#0E1715", UiStyle.dark); border.width: 1; border.color: UiStyle.color("#465750", UiStyle.dark) }
    Rectangle {
        id: titleBar; height: 42; anchors.left: parent.left; anchors.right: parent.right; color: UiStyle.color("#101A17", UiStyle.dark)
        MouseArea { anchors.fill: parent; anchors.rightMargin: 138; onPressed: root.startSystemMove(); onDoubleClicked: root.toggleMaximized() }
        Row {
            anchors.left: parent.left; anchors.leftMargin: 14; anchors.verticalCenter: parent.verticalCenter; spacing: 8
            Image { width: 20; height: 20; source: "image://theme/image-x-generic" }
            Text { text: qsTr("Photos"); color: root.foreground; font.family: "Inter"; font.pixelSize: 12; font.bold: true }
        }
        Row {
            anchors.right: parent.right
            Repeater {
                model: ["−", root.maximized ? "❐" : "□", "×"]
                delegate: Rectangle {
                    required property int index; required property string modelData
                    width: 46; height: 42
                    color: control.containsMouse ? (index === 2 ? "#C74646" : UiStyle.color("#304039", UiStyle.dark)) : "transparent"
                    Text { anchors.centerIn: parent; text: modelData; color: index === 2 && control.containsMouse ? "white" : root.foreground; font.pixelSize: 14 }
                    MouseArea { id: control; anchors.fill: parent; hoverEnabled: true; onClicked: index === 0 ? root.showMinimized() : index === 1 ? root.toggleMaximized() : root.close() }
                }
            }
        }
    }
    Rectangle {
        id: toolbar; anchors.left: parent.left; anchors.right: parent.right; anchors.top: titleBar.bottom; height: 52
        color: UiStyle.color("#111C19", UiStyle.dark)
        RowLayout {
            anchors.fill: parent; anchors.margins: 9; spacing: 6
            Action { text: qsTr("Open…"); tip: qsTr("Open photo (Ctrl+O)"); onClicked: picker.open() }
            Action { iconName: "go-previous"; tip: qsTr("Previous photo (←)"); enabled: PhotoLibrary.position > 1; onClicked: PhotoLibrary.previous() }
            Action { iconName: "go-next"; tip: qsTr("Next photo (→)"); enabled: PhotoLibrary.position < PhotoLibrary.count; onClicked: PhotoLibrary.next() }
            Text { Layout.fillWidth: true; text: PhotoLibrary.name; color: root.foreground; font.family: "Inter"; font.pixelSize: 12; elide: Text.ElideMiddle; horizontalAlignment: Text.AlignHCenter }
            Action { iconName: "zoom-out"; tip: qsTr("Zoom out (−)"); enabled: PhotoLibrary.url.toString().length > 0; onClicked: root.zoomBy(0.8) }
            Action { text: Math.round(root.actualZoom * 100) + "%"; tip: qsTr("Actual size (1)"); onClicked: { root.fitMode = false; root.zoom = 1 } }
            Action { iconName: "zoom-in"; tip: qsTr("Zoom in (+)"); enabled: PhotoLibrary.url.toString().length > 0; onClicked: root.zoomBy(1.25) }
            Action { text: qsTr("Fit"); tip: qsTr("Fit to window (0)"); onClicked: root.fitMode = true }
            Action { iconName: "object-rotate-right"; tip: qsTr("Rotate view (R)"); enabled: PhotoLibrary.url.toString().length > 0; onClicked: PhotoLibrary.rotate(1) }
            Action { iconName: "view-fullscreen"; tip: qsTr("Full screen (F11)"); onClicked: root.fullScreen() }
            Action { text: "⋯"; tip: qsTr("More actions"); onClicked: more.open() }
        }
    }
    Flickable {
        id: canvas; objectName: "photoCanvas"
        anchors.left: parent.left; anchors.right: parent.right; anchors.top: toolbar.bottom; anchors.bottom: status.top
        clip: true; boundsBehavior: Flickable.StopAtBounds
        contentWidth: Math.max(width, root.imageWidth * root.actualZoom)
        contentHeight: Math.max(height, root.imageHeight * root.actualZoom)
        Image {
            id: image; objectName: "photoImage"
            anchors.centerIn: parent
            width: Math.max(0, PhotoLibrary.pixelWidth * root.actualZoom); height: Math.max(0, PhotoLibrary.pixelHeight * root.actualZoom)
            source: PhotoLibrary.url; asynchronous: true; autoTransform: true; cache: false
            sourceSize: Qt.size(Math.min(8192, Math.max(0, PhotoLibrary.pixelWidth)), Math.min(8192, Math.max(0, PhotoLibrary.pixelHeight)))
            rotation: PhotoLibrary.rotation; fillMode: Image.PreserveAspectFit
            onStatusChanged: if (status === Image.Error) PhotoLibrary.decodeFailed()
        }
        ScrollBar.vertical: ScrollBar {}
        ScrollBar.horizontal: ScrollBar {}
        MouseArea {
            anchors.fill: parent; acceptedButtons: Qt.NoButton
            onWheel: function(wheel) { if (wheel.modifiers & Qt.ControlModifier) { root.zoomBy(wheel.angleDelta.y > 0 ? 1.25 : 0.8); wheel.accepted = true } else wheel.accepted = false }
        }
    }
    Column {
        anchors.centerIn: canvas; width: Math.min(460, canvas.width - 40); spacing: 14
        visible: PhotoLibrary.url.toString().length === 0 || PhotoLibrary.errorText.length > 0
        Image { anchors.horizontalCenter: parent.horizontalCenter; width: 60; height: 60; source: "image://theme/image-x-generic" }
        Text { width: parent.width; text: PhotoLibrary.errorText.length ? PhotoLibrary.errorText : qsTr("Open a photo or drop it here"); color: root.foreground; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.Wrap; font.family: "Inter"; font.pixelSize: 18 }
        Text { width: parent.width; text: qsTr("PNG · JPEG · WebP · BMP · GIF · TIFF"); color: root.secondary; horizontalAlignment: Text.AlignHCenter; font.pixelSize: 12 }
        Action { anchors.horizontalCenter: parent.horizontalCenter; text: qsTr("Open photo…"); onClicked: picker.open() }
    }
    DropArea { anchors.fill: canvas; onDropped: function(drop) { if (drop.hasUrls && drop.urls.length) { PhotoLibrary.open(drop.urls[0]); drop.acceptProposedAction() } } }
    Rectangle {
        id: status; height: 34; anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
        color: UiStyle.color("#101A17", UiStyle.dark)
        RowLayout {
            anchors.fill: parent; anchors.leftMargin: 14; anchors.rightMargin: 14
            Text { Layout.fillWidth: true; text: root.message.length ? root.message : PhotoLibrary.details; color: root.secondary; font.pixelSize: 11; elide: Text.ElideRight }
            Text { text: PhotoLibrary.count ? PhotoLibrary.position + " / " + PhotoLibrary.count : ""; color: root.secondary; font.pixelSize: 11 }
        }
    }
    Menu {
        id: more; x: root.width - width - 10; y: toolbar.y + toolbar.height; width: 210
        background: Rectangle { radius: 8; color: UiStyle.color("#17231F", UiStyle.dark); border.width: 1; border.color: UiStyle.color("#465750", UiStyle.dark) }
        Repeater {
            model: [qsTr("Copy image (Ctrl+C)"), qsTr("Rotate left"), qsTr("Set as wallpaper")]
            delegate: MenuItem {
                id: item; required property int index; required property string modelData
                enabled: PhotoLibrary.url.toString().length > 0 && !PhotoLibrary.copying
                text: modelData; height: 38
                contentItem: Text { text: item.text; color: root.foreground; font.pixelSize: 12 }
                background: Rectangle { radius: 5; color: item.highlighted ? UiStyle.color("#315F46", UiStyle.dark) : "transparent" }
                onTriggered: index === 0 ? PhotoLibrary.copyImage() : index === 1 ? PhotoLibrary.rotate(-1) : PhotoLibrary.useAsWallpaper()
            }
        }
    }
    MouseArea { anchors.left: parent.left; anchors.top: toolbar.bottom; anchors.bottom: parent.bottom; width: 5; visible: !root.maximized; cursorShape: Qt.SizeHorCursor; onPressed: root.startSystemResize(Qt.LeftEdge) }
    MouseArea { anchors.right: parent.right; anchors.top: toolbar.bottom; anchors.bottom: parent.bottom; width: 5; visible: !root.maximized; cursorShape: Qt.SizeHorCursor; onPressed: root.startSystemResize(Qt.RightEdge) }
    MouseArea { anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; height: 5; visible: !root.maximized; cursorShape: Qt.SizeVerCursor; onPressed: root.startSystemResize(Qt.BottomEdge) }
}
