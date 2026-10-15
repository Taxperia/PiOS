// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import Fidan.Shell

GlassPanel {
    id: root
    implicitWidth: 320
    implicitHeight: Controller.brightnessAvailable ? 386 : 342
    raised: true
    radius: 17
    clip: true

    property string pendingPowerAction: ""

    component ToggleTile: Rectangle {
        id: tile
        property string iconName: ""
        property string title: ""
        property string subtitle: ""
        property bool checked: false
        property bool available: true
        signal clicked
        width: 141
        height: 62
        radius: 11
        opacity: available ? 1.0 : 0.46
        color: checked ? "#315E46" : Controller.darkTheme
            ? (tileMouse.containsMouse ? "#34413C" : "#A6222D29")
            : (tileMouse.containsMouse ? "#D3E4DA" : "#E1EBE6")
        border.width: 1
        border.color: checked ? "#4D8968" : Theme.border(Controller.darkTheme)

        Row {
            anchors.fill: parent
            anchors.margins: 9
            spacing: 9
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: 34; height: 34; radius: 9
                color: tile.checked ? "#3A7857" : (Controller.darkTheme ? "#33423D" : "#CBDCD1")
                SystemIcon { anchors.centerIn: parent; width: 19; height: 19; name: tile.iconName; color: tile.checked ? "white" : Theme.text(Controller.darkTheme) }
            }
            Column {
                anchors.verticalCenter: parent.verticalCenter
                width: 78
                spacing: 3
                Text { width: parent.width; text: tile.title; elide: Text.ElideRight; color: tile.checked ? "white" : Theme.text(Controller.darkTheme); font.family: Theme.fontFamily; font.pixelSize: 10; font.weight: Font.DemiBold }
                Text { width: parent.width; text: tile.subtitle; elide: Text.ElideRight; color: tile.checked ? "#D5F0DF" : Theme.muted(Controller.darkTheme); font.family: Theme.fontFamily; font.pixelSize: 8 }
            }
        }
        MouseArea { id: tileMouse; anchors.fill: parent; enabled: tile.available; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: tile.clicked() }
        Behavior on color { ColorAnimation { duration: Theme.motionFast } }
    }

    component SettingSlider: Row {
        id: setting
        property string iconName: ""
        property real settingValue: 50
        property bool muted: false
        signal moved(real value)
        signal iconClicked
        width: 292
        height: 34
        spacing: 7

        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            width: 28; height: 28; radius: 7
            color: sliderIconMouse.containsMouse ? "#34433E" : "transparent"
            SystemIcon { anchors.centerIn: parent; width: 17; height: 17; name: setting.iconName; color: setting.muted ? Theme.muted(Controller.darkTheme) : Theme.text(Controller.darkTheme) }
            MouseArea { id: sliderIconMouse; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: setting.iconClicked() }
        }
        Slider {
            id: slider
            anchors.verticalCenter: parent.verticalCenter
            width: 218
            from: setting.iconName === "sun" ? 1 : 0
            to: 100
            value: setting.settingValue
            onMoved: setting.moved(value)
            background: Rectangle {
                x: slider.leftPadding
                y: slider.topPadding + slider.availableHeight / 2 - height / 2
                width: slider.availableWidth
                height: 6
                radius: 3
                color: Controller.darkTheme ? "#394640" : "#CDDAD2"
                Rectangle { width: slider.visualPosition * parent.width; height: parent.height; radius: 3; color: setting.muted ? "#718079" : Theme.accent }
            }
            handle: Rectangle {
                x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
                y: slider.topPadding + slider.availableHeight / 2 - height / 2
                width: 15; height: 15; radius: 8
                color: setting.muted ? "#A0ACA6" : Theme.accent
                border.width: 2
                border.color: "#DDFBE5"
            }
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            width: 32
            horizontalAlignment: Text.AlignRight
            text: Math.round(setting.settingValue) + "%"
            color: Theme.muted(Controller.darkTheme)
            font.family: Theme.fontFamily
            font.pixelSize: 9
        }
    }

    Column {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 10

        Row {
            width: parent.width
            height: 28
            Text { width: parent.width - closeButton.width; anchors.verticalCenter: parent.verticalCenter; text: qsTr("Quick Settings"); color: Theme.text(Controller.darkTheme); font.family: Theme.fontFamily; font.pixelSize: 13; font.weight: Font.Bold }
            IconButton { id: closeButton; compact: true; label: "×"; accessibleName: qsTr("Close"); onClicked: Controller.quickSettingsVisible = false }
        }

        Grid {
            columns: 2
            spacing: 10
            ToggleTile { iconName: "wifi"; title: qsTr("Wi-Fi"); available: Controller.wifiAvailable; subtitle: !available ? qsTr("Unavailable") : checked ? qsTr("On") : qsTr("Off"); checked: Controller.wifiEnabled; onClicked: Controller.toggleWifi() }
            ToggleTile { iconName: "bluetooth"; title: qsTr("Bluetooth"); available: Controller.bluetoothAvailable; subtitle: !available ? qsTr("Unavailable") : checked ? qsTr("On") : qsTr("Off"); checked: Controller.bluetoothEnabled; onClicked: Controller.toggleBluetooth() }
            ToggleTile { iconName: "moon"; title: qsTr("Dark Mode"); subtitle: checked ? qsTr("On") : qsTr("Off"); checked: Controller.darkTheme; onClicked: Controller.darkTheme = !Controller.darkTheme }
            ToggleTile {
                iconName: "bolt"
                title: qsTr("Power Mode")
                available: Controller.powerProfilesAvailable
                subtitle: available ? Controller.powerProfile : qsTr("Unavailable")
                checked: available
                onClicked: Controller.cyclePowerProfile()
            }
        }

        Rectangle { width: parent.width; height: 1; color: Theme.border(Controller.darkTheme); opacity: 0.7 }

        SettingSlider {
            visible: Controller.brightnessAvailable
            iconName: "sun"
            settingValue: Controller.brightness
            onMoved: value => Controller.brightness = Math.round(value)
        }
        SettingSlider {
            visible: Controller.audioAvailable
            iconName: "volume"
            muted: Controller.volumeMuted
            settingValue: Controller.volumeMuted ? 0 : Controller.volume
            onMoved: value => Controller.volume = Math.round(value)
            onIconClicked: Controller.toggleVolumeMute()
        }

        Rectangle { width: parent.width; height: 1; color: Theme.border(Controller.darkTheme); opacity: 0.7 }

        Row {
            width: parent.width
            spacing: 8
            Repeater {
                model: [
                    { title: qsTr("Lock"), icon: "lock", action: "lock" },
                    { title: qsTr("Restart"), icon: "restart", action: "restart" },
                    { title: qsTr("Shut Down"), icon: "power", action: "shutdown" }
                ]
                Rectangle {
                    required property var modelData
                    property bool available: modelData.action === "lock" ? Controller.lockAvailable : Controller.powerActionsAvailable
                    width: (parent.width - 16) / 3
                    height: 62
                    radius: 10
                    color: Controller.darkTheme
                        ? (actionMouse.containsMouse ? "#35483F" : "#9A202A27")
                        : (actionMouse.containsMouse ? "#D3E4DA" : "#E1EBE6")
                    border.width: 1
                    border.color: Theme.border(Controller.darkTheme)
                    opacity: available ? 1.0 : 0.42
                    Column {
                        anchors.centerIn: parent
                        spacing: 6
                        SystemIcon { anchors.horizontalCenter: parent.horizontalCenter; width: 18; height: 18; name: modelData.icon; color: Theme.text(Controller.darkTheme) }
                        Text { anchors.horizontalCenter: parent.horizontalCenter; text: modelData.title; color: Theme.muted(Controller.darkTheme); font.family: Theme.fontFamily; font.pixelSize: 8 }
                    }
                    MouseArea {
                        id: actionMouse
                        anchors.fill: parent
                        enabled: parent.available
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (modelData.action === "lock") {
                                Controller.lockSession()
                            } else {
                                root.pendingPowerAction = modelData.action
                            }
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        visible: root.pendingPowerAction.length > 0
        z: 20
        radius: root.radius
        color: Controller.darkTheme ? "#F20B1311" : "#FAF8FBF9"

        Column {
            anchors.centerIn: parent
            width: parent.width - 46
            spacing: 14
            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                width: 54; height: 54; radius: 16
                color: "#2C4038"
                SystemIcon { anchors.centerIn: parent; width: 27; height: 27; name: root.pendingPowerAction === "restart" ? "restart" : "power"; color: Theme.accent }
            }
            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                text: root.pendingPowerAction === "restart" ? qsTr("Restart this device?") : qsTr("Shut down this device?")
                color: Theme.text(Controller.darkTheme)
                font.family: Theme.fontFamily
                font.pixelSize: 16
                font.weight: Font.Bold
            }
            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                text: qsTr("Save your work before continuing.")
                color: Theme.muted(Controller.darkTheme)
                font.family: Theme.fontFamily
                font.pixelSize: 10
            }
            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 10
                Rectangle {
                    width: 105; height: 36; radius: 8; color: Controller.darkTheme ? (cancelMouse.containsMouse ? "#35443F" : "#26332F") : (cancelMouse.containsMouse ? "#D3E4DA" : "#E1EBE6"); border.width: 1; border.color: Theme.border(Controller.darkTheme)
                    Text { anchors.centerIn: parent; text: qsTr("Cancel"); color: Theme.text(Controller.darkTheme); font.family: Theme.fontFamily; font.pixelSize: 10 }
                    MouseArea { id: cancelMouse; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: root.pendingPowerAction = "" }
                }
                Rectangle {
                    width: 105; height: 36; radius: 8; color: confirmMouse.containsMouse ? "#D85A5A" : "#B84343"
                    Text { anchors.centerIn: parent; text: root.pendingPowerAction === "restart" ? qsTr("Restart") : qsTr("Shut Down"); color: "white"; font.family: Theme.fontFamily; font.pixelSize: 10; font.weight: Font.DemiBold }
                    MouseArea {
                        id: confirmMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            const action = root.pendingPowerAction
                            root.pendingPowerAction = ""
                            if (Controller.mockMode) {
                                Notifications.Notify("Project Fidan", 0, "", action === "restart" ? qsTr("Restart requested") : qsTr("Shutdown requested"), qsTr("The real device will perform this action."), [], {}, 3500)
                            }
                            Controller.requestPowerAction(action)
                            Controller.quickSettingsVisible = false
                        }
                    }
                }
            }
        }
    }
}
