// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
Button {
    id: button
    property bool accent: false
    property string iconName: ""
    property string tip: ""
    implicitHeight: 38
    implicitWidth: Math.max(38, label.implicitWidth + (iconName.length ? 50 : 26))
    opacity: enabled ? 1 : 0.4
    Accessible.name: tip.length ? tip : text
    font.family: "Inter"
    contentItem: Item {
        Row {
            anchors.centerIn: parent; spacing: 8
            Image { visible: button.iconName.length > 0; width: visible ? 18 : 0; height: 18; anchors.verticalCenter: parent.verticalCenter; source: visible ? "image://theme/"+button.iconName+(UiStyle.dark?"?theme=dark":"?theme=light") : "" }
            Text { id: label; text: button.text; color: button.accent ? "#082C17" : UiStyle.color("#F2F7F4",UiStyle.dark); anchors.verticalCenter: parent.verticalCenter; font: button.font }
        }
    }
    background: Rectangle { radius: 8; color: button.accent ? UiStyle.accent : UiStyle.color(button.down ? "#315F46" : button.hovered ? "#304039" : "#1A2823",UiStyle.dark); border.width: button.activeFocus ? 2 : 0; border.color: UiStyle.accent }
    ToolTip.visible: hovered && tip.length > 0; ToolTip.text: tip; ToolTip.delay: 500
}
