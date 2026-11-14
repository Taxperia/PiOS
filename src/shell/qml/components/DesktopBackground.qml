// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import Fidan.Shell

Item {
    Image {
        anchors.fill: parent
        source: Controller.wallpaperUrl.length ? Controller.wallpaperUrl : Product.wallpaperUrl
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
        cache: true
    }
    Rectangle {
        anchors.fill: parent
        color: Controller.darkTheme ? Controller.themeColor("#18050D0A", Controller.themeMode, Controller.darkTheme) : Controller.themeColor("#08FFFFFF", Controller.themeMode, Controller.darkTheme)
        Behavior on color { ColorAnimation { duration: Theme.motionNormal } }
    }
}

