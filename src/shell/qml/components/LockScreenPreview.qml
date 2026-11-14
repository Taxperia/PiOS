// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import Fidan.Shell

Item {
    id: root

    Image { anchors.fill: parent; source: Controller.lockWallpaperUrl.length ? Controller.lockWallpaperUrl : Controller.wallpaperUrl.length ? Controller.wallpaperUrl : Product.wallpaperUrl; fillMode: Image.PreserveAspectCrop }
    Rectangle { anchors.fill: parent; color: Controller.themeColor("#72040B08", Controller.themeMode, Controller.darkTheme) }

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
            color: Controller.themeColor("#D7E0DB", Controller.themeMode, Controller.darkTheme); font.family: Theme.fontFamily; font.pixelSize: 14
        }
        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: 76; height: 76; radius: 38; color: Controller.themeColor("#E8F0EC", Controller.themeMode, Controller.darkTheme)
            SystemIcon { anchors.centerIn: parent; width: 38; height: 38; name: "home"; color: Controller.themeColor("#40564B", Controller.themeMode, Controller.darkTheme) }
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
            background: Rectangle { radius: 10; color: Controller.themeColor("#CA17221E", Controller.themeMode, Controller.darkTheme); border.width: 1; border.color: Theme.border(true) }
        }
        Button {
            width: parent.width; height: 40; text: qsTr("Enter desktop preview")
            onClicked: Controller.unlockPreview()
            background: Rectangle { radius: 10; color: Theme.accent }
            contentItem: Text { text: parent.text; color: Controller.themeColor("#07110B", Controller.themeMode, Controller.darkTheme); font.family: Theme.fontFamily; font.pixelSize: 12; font.weight: Font.Bold; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
        }
        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            text: qsTr("This screen only previews the design. Real locking uses gtklock, ext-session-lock-v1 and PAM in the Labwc session.")
            color: Controller.themeColor("#B7C5BE", Controller.themeMode, Controller.darkTheme); font.family: Theme.fontFamily; font.pixelSize: 9
        }
    }
}
