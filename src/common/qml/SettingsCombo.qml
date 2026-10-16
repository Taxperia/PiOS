// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Controls as QQC
import QtQuick.Layouts

QQC.ComboBox {
        id: choice; Layout.fillWidth: true; implicitHeight: 36; font.family: "Inter"; font.pixelSize: 12
        leftPadding: 11; rightPadding: 28; opacity: enabled ? 1 : 0.4
        palette.text: Controller.darkTheme ? "#F2F7F4" : "#18261F"; palette.base: Controller.darkTheme ? "#19312E" : "#F7FCF9"
        contentItem: Text { text: choice.displayText; color: choice.palette.text; font: choice.font; verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight }
        indicator: Text { anchors.right: parent.right; anchors.rightMargin: 10; anchors.verticalCenter: parent.verticalCenter; text: "⌄"; color: choice.palette.text }
        background: Rectangle { radius: 7; color: choice.palette.base; border.width: 1; border.color: Controller.darkTheme ? "#3B5650" : "#BDCEC4" }
        // A standalone component keeps popup delegates out of Qt 6.8's nested
        // inline-component incubator, which aborts when this menu is opened.
        popup: QQC.Popup {
            objectName: choice.objectName + "-popup"
            popupType: QQC.Popup.Item
            y: choice.height+4; width: choice.width; padding: 4
            implicitHeight: Math.min(240, options.contentHeight+8)
            background: Rectangle { radius: 8; color: choice.palette.base; border.width: 1; border.color: Controller.darkTheme ? "#3B5650" : "#BDCEC4" }
            contentItem: ListView {
                id: options; clip: true; implicitHeight: contentHeight
                model: choice.model; currentIndex: choice.highlightedIndex
                delegate: QQC.ItemDelegate {
                    id: option
                    required property int index
                    required property var modelData
                    text: String(modelData)
                    objectName: choice.objectName + "-option-" + index
                    width: options.width; height: 36; highlighted: choice.currentIndex === index
                    contentItem: Text { text: option.text; color: choice.palette.text; font: choice.font; verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight }
                    background: Rectangle { radius: 5; color: parent.highlighted || parent.hovered ? Qt.alpha(Controller.accentColor,0.25) : "transparent" }
                    onClicked: { choice.currentIndex=index; choice.activated(index); choice.popup.close() }
                }
                ScrollIndicator.vertical: ScrollIndicator {}
            }
        }
    }
