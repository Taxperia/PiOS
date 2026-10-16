// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

ApplicationWindow {
    id: root
    objectName: "utilityWindow"
    width: AppKind === "calculator" ? 720 : AppKind === "pdf" ? 1100 : 930
    height: AppKind === "calculator" ? 650 : 700
    minimumWidth: AppKind === "calculator" ? 520 : 760
    minimumHeight: 560
    visible: true; title: "PiOS " + AppTitle
    flags: Qt.Window | Qt.FramelessWindowHint
    color: "transparent"
    font.family: "Inter"
    readonly property bool maximized: visibility === Window.Maximized || visibility === Window.FullScreen
    function toggleMaximized() { maximized ? showNormal() : showMaximized() }
    palette.window: UiStyle.color("#0E1715",UiStyle.dark)
    palette.windowText: UiStyle.color("#F2F7F4",UiStyle.dark)
    palette.text: UiStyle.color("#F2F7F4",UiStyle.dark)
    palette.base: UiStyle.color("#17231F",UiStyle.dark)
    palette.button: UiStyle.color("#1A2823",UiStyle.dark)
    palette.buttonText: UiStyle.color("#F2F7F4",UiStyle.dark)
    palette.highlight: UiStyle.accent
    palette.highlightedText: "#082C17"
    background: Rectangle { radius: root.maximized ? 0 : 12; color: UiStyle.color("#0E1715",UiStyle.dark); border.color: UiStyle.color("#465750",UiStyle.dark) }
    header: Rectangle {
        height: 46; color: UiStyle.color("#101A17",UiStyle.dark)
        MouseArea { anchors.fill: parent; anchors.rightMargin: 138; onPressed: root.startSystemMove(); onDoubleClicked: root.toggleMaximized() }
        Row {
            anchors.left: parent.left; anchors.leftMargin: 16; anchors.verticalCenter: parent.verticalCenter; spacing: 10
            Image { width: 22; height: 22; source: "image://theme/org.fidan."+(AppKind === "pdf" ? "Pdf" : AppKind === "calculator" ? "Calculator" : AppKind === "clock" ? "Clock" : "Calendar") }
            Text { text: "PiOS " + AppTitle; color: root.palette.text; font.pixelSize: 13; font.weight: Font.DemiBold }
        }
        Row {
            anchors.right: parent.right
            Repeater {
                model: ["−",root.maximized ? "❐" : "□","×"]
                delegate: Button {
                    id: windowButton
                    required property int index
                    required property string modelData
                    width: 46; height: 46
                    Accessible.name: index === 0 ? "Küçült" : index === 1 ? "Büyüt / geri al" : "Kapat"
                    contentItem: Text { text: windowButton.modelData; color: windowButton.hovered && windowButton.index === 2 ? "white" : root.palette.text; font.pixelSize: 17; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    background: Rectangle { color: windowButton.hovered ? windowButton.index === 2 ? "#C74646" : UiStyle.color("#304039",UiStyle.dark) : "transparent" }
                    onClicked: index === 0 ? root.showMinimized() : index === 1 ? root.toggleMaximized() : root.close()
                }
            }
        }
    }
    Loader { id: page; anchors.fill: parent; anchors.margins: 1; source: AppKind === "pdf" ? "PdfViewer.qml" : AppKind === "clock" ? "Clock.qml" : AppKind === "calendar" ? "Calendar.qml" : "Calculator.qml" }
    MouseArea { anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom; width: 5; visible: !root.maximized; cursorShape: Qt.SizeHorCursor; onPressed: root.startSystemResize(Qt.LeftEdge) }
    MouseArea { anchors.right: parent.right; anchors.top: parent.top; anchors.bottom: parent.bottom; width: 5; visible: !root.maximized; cursorShape: Qt.SizeHorCursor; onPressed: root.startSystemResize(Qt.RightEdge) }
    MouseArea { anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; height: 5; visible: !root.maximized; cursorShape: Qt.SizeVerCursor; onPressed: root.startSystemResize(Qt.BottomEdge) }
}
