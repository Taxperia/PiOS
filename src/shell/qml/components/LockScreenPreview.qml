// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import Fidan.Shell

Item {
    id: root

    Image { anchors.fill: parent; source: Controller.lockWallpaperUrl.length ? Controller.lockWallpaperUrl : Controller.wallpaperUrl.length ? Controller.wallpaperUrl : Product.wallpaperUrl; fillMode: Image.PreserveAspectCrop }
    Rectangle { anchors.fill: parent; color: "#72040B08" }

    Column {
        anchors.centerIn: parent
        width: 350
        spacing: 12

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: Controller.clockText
            color: "white"; font.family: Theme.fontFamily; font.pixelSize: 58; font.weight: Font.Light
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: Controller.dateText
            color: "#D7E0DB"; font.family: Theme.fontFamily; font.pixelSize: 14
        }
        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: 76; height: 76; radius: 38; color: "#E8F0EC"
            SystemIcon { anchors.centerIn: parent; width: 38; height: 38; name: "home"; color: "#40564B" }
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Fidan User"); color: "white"; font.family: Theme.fontFamily; font.pixelSize: 14; font.weight: Font.DemiBold
        }
        TextField {
            width: parent.width; height: 42
            placeholderText: qsTr("Authentication is disabled in visual preview")
            echoMode: TextInput.Password
            enabled: false
            color: Theme.text(true); placeholderTextColor: Theme.muted(true)
            font.family: Theme.fontFamily
            background: Rectangle { radius: 10; color: "#CA17221E"; border.width: 1; border.color: Theme.border(true) }
        }
        Button {
            width: parent.width; height: 40; text: qsTr("Enter desktop preview")
            onClicked: Controller.unlockPreview()
            background: Rectangle { radius: 10; color: Theme.accent }
            contentItem: Text { text: parent.text; color: "#07110B"; font.family: Theme.fontFamily; font.pixelSize: 12; font.weight: Font.Bold; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
        }
        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            text: qsTr("This screen only previews the design. Real locking uses gtklock, ext-session-lock-v1 and PAM in the Labwc session.")
            color: "#B7C5BE"; font.family: Theme.fontFamily; font.pixelSize: 9
        }
    }
}
