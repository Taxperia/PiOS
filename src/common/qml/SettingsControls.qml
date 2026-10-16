// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Controls as QQC
import QtQuick.Layouts

Item {
    component Card: Rectangle {
        id: card
        property int contentSpacing: 8
        default property alias contents: body.data
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignTop
        implicitHeight: body.implicitHeight + 28
        radius: 11; color: Controller.darkTheme ? "#C10F2423" : "#ECF7FCF9"
        border.width: 1; border.color: Controller.darkTheme ? "#3B5650" : "#BDCEC4"
        ColumnLayout { id: body; x: 14; y: 14; width: parent.width - 28; spacing: card.contentSpacing }
    }
    component Label: Text {
        Layout.fillWidth: true; wrapMode: Text.Wrap
        color: Controller.darkTheme ? "#F2F7F4" : "#18261F"
        font.family: "Inter"; font.pixelSize: 12
    }
    component Caption: Text {
        Layout.fillWidth: true; wrapMode: Text.Wrap
        color: Controller.darkTheme ? "#9FB6B3" : "#586B60"
        font.family: "Inter"; font.pixelSize: 11
    }
    component Title: Text {
        Layout.fillWidth: true; wrapMode: Text.Wrap
        color: Controller.darkTheme ? "#F2F7F4" : "#18261F"
        font.family: "Inter"; font.pixelSize: 15; font.weight: Font.DemiBold
    }
    component Icon: Image {
        property string name: "preferences-system-symbolic"
        Layout.preferredWidth: 24; Layout.preferredHeight: 24
        sourceSize: Qt.size(32,32); fillMode: Image.PreserveAspectFit
        source: "image://theme/" + name + (Controller.darkTheme ? "?theme=dark" : "?theme=light")
    }
    component Button: QQC.Button {
        id: button; property bool primary: false
        implicitHeight: 35; leftPadding: 14; rightPadding: 14
        font.family: "Inter"; font.pixelSize: 11; opacity: enabled ? 1 : 0.4
        contentItem: Text { text: button.text; color: Controller.darkTheme ? "#F2F7F4" : "#18261F"; font: button.font; elide: Text.ElideRight; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
        background: Rectangle {
            radius: 7; border.width: 1
            border.color: button.primary ? Controller.accentColor : (Controller.darkTheme ? "#3B5650" : "#BDCEC4")
            color: button.primary ? Qt.alpha(Controller.accentColor, button.hovered ? 0.4 : 0.22) : (Controller.darkTheme ? (button.hovered ? "#334E49" : "#213A37") : (button.hovered ? "#D2E5D9" : "#E4EEE8"))
        }
    }
    component Option: QQC.Button {
        id: option; property bool selected: false; property string iconName: ""; property string dockSide: ""
        Layout.fillWidth: true; Layout.preferredWidth: 80; Layout.minimumWidth: 0; implicitWidth: 80; implicitHeight: 62; opacity: enabled ? 1 : 0.4
        contentItem: Column {
            spacing: 7
            Item {
                anchors.horizontalCenter: parent.horizontalCenter; width:22; height:22; visible: option.iconName.length>0 || option.dockSide.length>0
                Image { anchors.fill:parent; visible:option.dockSide.length===0; source:option.iconName.length ? "image://theme/"+option.iconName+(Controller.darkTheme?"?theme=dark":"?theme=light") : "" }
                Rectangle { anchors.fill:parent; visible:option.dockSide.length>0; radius:2; color:"transparent"; border.width:1; border.color:Controller.darkTheme?"#F2F7F4":"#18261F"; Rectangle { x:option.dockSide==="right"?15:3; y:option.dockSide==="bottom"?15:3; width:option.dockSide==="bottom"?16:4; height:option.dockSide==="bottom"?4:16; radius:1; color:Controller.darkTheme?"#F2F7F4":"#18261F" } }
            }
            Text { width: parent.width; horizontalAlignment: Text.AlignHCenter; text: option.text; color: Controller.darkTheme ? "#F2F7F4" : "#18261F"; font.family: "Inter"; font.pixelSize: 11 }
        }
        background: Rectangle { radius: 8; color: option.selected ? Qt.alpha(Controller.accentColor, 0.2) : (Controller.darkTheme ? "#263C39" : "#E3EEE7"); border.width: option.selected ? 2 : 1; border.color: option.selected ? Controller.accentColor : (Controller.darkTheme ? "#3B5650" : "#BDCEC4") }
    }
    component Toggle: RowLayout {
        id: row; property string label; property string hint: ""; property bool checked: false
        signal toggled(bool value)
        Layout.fillWidth: true; spacing: 14
        ColumnLayout { Layout.fillWidth: true; spacing: 4; Label { text: row.label } Caption { text: row.hint; visible: text.length > 0 } }
        Switch {
            id: toggle; checked: row.checked; onToggled: row.toggled(checked)
            opacity: enabled ? 1 : 0.4
            indicator: Rectangle { implicitWidth: 38; implicitHeight: 22; x: toggle.leftPadding; y: toggle.topPadding + toggle.availableHeight/2-height/2; radius: 11; color: toggle.checked ? Controller.accentColor : (Controller.darkTheme ? "#4C625C" : "#BACFC1"); Rectangle { width: 18; height: 18; x: toggle.checked ? 18 : 2; y: 2; radius: 9; color: "#F4FBF7" } }
        }
    }
    component Slider: QQC.Slider {
        id: slider; Layout.fillWidth: true; implicitHeight: 28; opacity: enabled ? 1 : 0.4
        background: Rectangle { x: slider.leftPadding; y: slider.topPadding + slider.availableHeight/2-3; width: slider.availableWidth; height: 6; radius: 3; color: Controller.darkTheme ? "#3A5350" : "#BFD2C6"; Rectangle { width: slider.visualPosition*parent.width; height: 6; radius: 3; color: Controller.accentColor } }
        handle: Rectangle { x: slider.leftPadding + slider.visualPosition*(slider.availableWidth-width); y: slider.topPadding + slider.availableHeight/2-height/2; width: 17; height: 17; radius: 9; color: Controller.accentColor; border.width: 1; border.color: "#D1EBDD" }
    }
    component Combo: SettingsCombo { }
    component Link: Rectangle {
        id: link; property string label; property string hint: ""; property string value: ""; property string iconName: "go-next-symbolic"
        signal triggered()
        Layout.fillWidth: true; implicitHeight: (Controller.interfaceScale > 1.25 ? 82 : Controller.interfaceScale < 1.25 ? 64 : 74)
        radius: 10; color: hover.containsMouse ? Qt.alpha(Controller.accentColor,0.14) : (Controller.darkTheme ? "#B40E2324" : "#EAF9FCFA"); border.width: 1; border.color: Controller.darkTheme ? "#354F4B" : "#BDCEC4"
        activeFocusOnTab: enabled; opacity: enabled ? 1 : 0.45
        Accessible.role: Accessible.Button; Accessible.name: label
        RowLayout {
            anchors.fill: parent; anchors.margins: 14; spacing: 14
            Icon { name: link.iconName; Layout.preferredWidth: 27; Layout.preferredHeight: 27 }
            ColumnLayout { Layout.fillWidth: true; spacing: 5; Label { text: link.label; font.pixelSize: 13 } Caption { text: link.hint; visible: text.length > 0 } }
            Caption { Layout.fillWidth: false; Layout.maximumWidth: link.width*0.28; text: link.value; horizontalAlignment: Text.AlignRight; elide: Text.ElideRight; maximumLineCount: 2 }
            Label { Layout.fillWidth: false; text: "›"; font.pixelSize: 23 }
        }
        MouseArea { id: hover; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: link.triggered() }
        Keys.onReturnPressed: triggered(); Keys.onSpacePressed: triggered()
    }
    component InfoRow: Item {
        id: row; property string label; property string value; property string iconName: ""; Layout.fillWidth: true; implicitHeight: 30
        RowLayout { anchors.fill: parent; spacing: 10; Icon { name: row.iconName; visible: row.iconName.length > 0; Layout.preferredWidth: 16; Layout.preferredHeight: 16 } Caption { text: row.label; Layout.preferredWidth: parent.width*0.38 } Label { text: row.value || "—"; horizontalAlignment: Text.AlignRight; font.pixelSize: 11; elide: Text.ElideRight; maximumLineCount: 2 } }
        Rectangle { anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; height: 1; color: Controller.darkTheme ? "#243D39" : "#D8E5DD" }
    }
}
