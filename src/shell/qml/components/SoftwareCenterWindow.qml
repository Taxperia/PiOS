// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import Fidan.Shell

GlassPanel {
    id: root
    raised: true
    radius: 13
    clip: true

    signal closeRequested
    signal minimizeRequested
    signal maximizeRequested

    property bool maximized: false
    property string selectedCategory: "Featured"
    property string searchText: ""
    property string activityText: qsTr("Discover apps made for Project Fidan")

    function matches(title, category, installed, updateAvailable, featured) {
        const query = searchText.trim().toLowerCase()
        if (query.length > 0 && title.toLowerCase().indexOf(query) < 0
                && category.toLowerCase().indexOf(query) < 0)
            return false
        if (selectedCategory === "All Apps") return true
        if (selectedCategory === "Installed") return installed
        if (selectedCategory === "Updates") return updateAvailable
        if (selectedCategory === "Featured") return featured
        return selectedCategory === category
    }

    ListModel {
        id: softwareModel
        ListElement { title: "Code Studio"; subtitle: "Lightweight IDE"; appCategory: "Development"; iconName: "terminal"; iconColor: "#48A9FF"; installed: false; installing: false; updateAvailable: false; featured: true }
        ListElement { title: "Web Browser"; subtitle: "Fast & private"; appCategory: "All Apps"; iconName: "globe"; iconColor: "#5E8DFF"; installed: true; installing: false; updateAvailable: true; featured: true }
        ListElement { title: "Media Player"; subtitle: "Video, music & more"; appCategory: "Multimedia"; iconName: "play"; iconColor: "#F24E5E"; installed: false; installing: false; updateAvailable: false; featured: true }
        ListElement { title: "GPIO Tools"; subtitle: "Control your hardware"; appCategory: "Tools & Makers"; iconName: "chip"; iconColor: "#A6E46F"; installed: false; installing: false; updateAvailable: false; featured: true }
        ListElement { title: "Terminal Plus"; subtitle: "A modern terminal"; appCategory: "Development"; iconName: "terminal"; iconColor: "#75D5A0"; installed: true; installing: false; updateAvailable: false; featured: true }
        ListElement { title: "Image Writer"; subtitle: "Flash OS images"; appCategory: "Tools & Makers"; iconName: "store"; iconColor: "#A986E8"; installed: false; installing: false; updateAvailable: false; featured: true }
        ListElement { title: "Study Notes"; subtitle: "Write and organise"; appCategory: "Education"; iconName: "apps"; iconColor: "#F5BA59"; installed: false; installing: false; updateAvailable: false; featured: false }
        ListElement { title: "Photo Viewer"; subtitle: "Browse your pictures"; appCategory: "Multimedia"; iconName: "folder"; iconColor: "#57CFA0"; installed: true; installing: false; updateAvailable: true; featured: false }
        ListElement { title: "System Monitor"; subtitle: "Watch device health"; appCategory: "System"; iconName: "temperature"; iconColor: "#70CDE3"; installed: true; installing: false; updateAvailable: false; featured: false }
    }

    Rectangle {
        id: titleBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 42
        color: "#ED0E1715"

        MouseArea {
            anchors.left: parent.left
            anchors.right: windowControls.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            cursorShape: root.maximized ? Qt.ArrowCursor : Qt.SizeAllCursor
            drag.target: root.maximized ? null : root
            drag.minimumX: 0
            drag.maximumX: Math.max(0, root.parent ? root.parent.width - root.width : 0)
            drag.minimumY: Theme.panelHeight
            drag.maximumY: Math.max(Theme.panelHeight, root.parent ? root.parent.height - root.height : Theme.panelHeight)
            onDoubleClicked: root.maximizeRequested()
        }

        Row {
            anchors.left: parent.left
            anchors.leftMargin: 14
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8
            SystemIcon { width: 16; height: 16; name: "store"; color: Theme.accent }
            Text {
                text: qsTr("Software Center")
                color: Theme.text(Controller.darkTheme)
                font.family: Theme.fontFamily
                font.pixelSize: 11
                font.weight: Font.DemiBold
            }
        }

        Row {
            id: windowControls
            anchors.right: parent.right
            anchors.top: parent.top
            height: parent.height

            component WindowButton: Rectangle {
                id: control
                property string symbol: ""
                property bool destructive: false
                signal clicked
                width: 42
                height: titleBar.height
                color: controlMouse.containsMouse ? (destructive ? "#C74646" : "#34413D") : "transparent"
                Text { anchors.centerIn: parent; text: control.symbol; color: Theme.text(Controller.darkTheme); font.family: "sans-serif"; font.pixelSize: 13 }
                MouseArea { id: controlMouse; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: control.clicked() }
            }

            WindowButton { symbol: "−"; onClicked: root.minimizeRequested() }
            WindowButton { symbol: root.maximized ? "❐" : "□"; onClicked: root.maximizeRequested() }
            WindowButton { symbol: "×"; destructive: true; onClicked: root.closeRequested() }
        }
    }

    Rectangle {
        id: sidebar
        anchors.left: parent.left
        anchors.top: titleBar.bottom
        anchors.bottom: footer.top
        width: 166
        color: "#E50E1715"
        border.width: 1
        border.color: "#304B5550"

        Column {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 4

            Text {
                leftPadding: 8
                bottomPadding: 7
                text: qsTr("Browse")
                color: Theme.muted(Controller.darkTheme)
                font.family: Theme.fontFamily
                font.pixelSize: 9
                font.weight: Font.DemiBold
            }

            Repeater {
                model: [
                    { title: "Featured", icon: "home" },
                    { title: "All Apps", icon: "apps" },
                    { title: "Development", icon: "terminal" },
                    { title: "Multimedia", icon: "play" },
                    { title: "Tools & Makers", icon: "chip" },
                    { title: "Education", icon: "apps" },
                    { title: "System", icon: "settings" }
                ]
                Rectangle {
                    required property var modelData
                    width: parent.width
                    height: 34
                    radius: 7
                    color: root.selectedCategory === modelData.title ? "#315F46" : (categoryMouse.containsMouse ? "#25342F" : "transparent")
                    Row {
                        anchors.left: parent.left
                        anchors.leftMargin: 9
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 9
                        SystemIcon { anchors.verticalCenter: parent.verticalCenter; width: 14; height: 14; name: modelData.icon; color: root.selectedCategory === modelData.title ? Theme.accent : Theme.muted(Controller.darkTheme) }
                        Text { anchors.verticalCenter: parent.verticalCenter; text: modelData.title; color: Theme.text(Controller.darkTheme); font.family: Theme.fontFamily; font.pixelSize: 10 }
                    }
                    MouseArea {
                        id: categoryMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.selectedCategory = modelData.title
                    }
                }
            }

            Rectangle { width: parent.width; height: 1; color: Theme.border(Controller.darkTheme) }

            Repeater {
                model: [
                    { title: "Installed", icon: "folder" },
                    { title: "Updates", icon: "restart" }
                ]
                Rectangle {
                    required property var modelData
                    width: parent.width
                    height: 34
                    radius: 7
                    color: root.selectedCategory === modelData.title ? "#315F46" : (libraryMouse.containsMouse ? "#25342F" : "transparent")
                    Row {
                        anchors.left: parent.left; anchors.leftMargin: 9
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 9
                        SystemIcon { anchors.verticalCenter: parent.verticalCenter; width: 14; height: 14; name: modelData.icon; color: root.selectedCategory === modelData.title ? Theme.accent : Theme.muted(Controller.darkTheme) }
                        Text { anchors.verticalCenter: parent.verticalCenter; text: modelData.title; color: Theme.text(Controller.darkTheme); font.family: Theme.fontFamily; font.pixelSize: 10 }
                    }
                    Rectangle {
                        visible: modelData.title === "Updates"
                        anchors.right: parent.right
                        anchors.rightMargin: 8
                        anchors.verticalCenter: parent.verticalCenter
                        width: 20; height: 20; radius: 10; color: Theme.accent
                        Text { anchors.centerIn: parent; text: "2"; color: "#0B2015"; font.family: Theme.fontFamily; font.pixelSize: 9; font.weight: Font.Bold }
                    }
                    MouseArea {
                        id: libraryMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.selectedCategory = modelData.title
                    }
                }
            }
        }
    }

    Rectangle {
        id: content
        anchors.left: sidebar.right
        anchors.right: parent.right
        anchors.top: titleBar.bottom
        anchors.bottom: footer.top
        color: "#F00B1412"

        TextField {
            id: search
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 13
            height: 34
            leftPadding: 35
            rightPadding: 12
            placeholderText: qsTr("Search apps, tools, and more…")
            color: Theme.text(Controller.darkTheme)
            placeholderTextColor: Theme.muted(Controller.darkTheme)
            font.family: Theme.fontFamily
            font.pixelSize: 10
            selectByMouse: true
            onTextChanged: root.searchText = text
            background: Rectangle {
                radius: 7
                color: "#B9232E2A"
                border.width: search.activeFocus ? 1 : 0
                border.color: Theme.accent
            }
            SystemIcon { anchors.left: parent.left; anchors.leftMargin: 11; anchors.verticalCenter: parent.verticalCenter; width: 14; height: 14; name: "search"; color: Theme.muted(Controller.darkTheme) }
        }

        Flickable {
            id: catalogue
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: search.bottom
            anchors.bottom: parent.bottom
            anchors.topMargin: 10
            clip: true
            contentWidth: width
            contentHeight: catalogueColumn.height + 20
            boundsBehavior: Flickable.StopAtBounds

            Column {
                id: catalogueColumn
                x: 13
                width: catalogue.width - 26
                spacing: 11

                Rectangle {
                    width: parent.width
                    height: 132
                    radius: 11
                    visible: root.selectedCategory === "Featured" && root.searchText.length === 0
                    color: "#183E31"
                    border.width: 1
                    border.color: "#356B54"
                    clip: true

                    Rectangle { x: parent.width * 0.58; y: -42; width: 230; height: 230; radius: 115; color: "#17382D"; rotation: 12 }
                    Rectangle { x: parent.width * 0.72; y: -18; width: 170; height: 170; radius: 85; color: "#1D4938"; rotation: -16 }
                    Column {
                        anchors.left: parent.left
                        anchors.leftMargin: 20
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width * 0.56
                        spacing: 5
                        Text { text: qsTr("Tools for a bigger imagination"); color: "white"; font.family: Theme.fontFamily; font.pixelSize: 18; font.weight: Font.Bold }
                        Text { width: parent.width; text: qsTr("Apps for coding, making, learning and more on Raspberry Pi 5."); color: "#D3E3DB"; font.family: Theme.fontFamily; font.pixelSize: 10; wrapMode: Text.Wrap }
                        Rectangle {
                            width: 100; height: 30; radius: 6; color: Theme.accent
                            Text { anchors.centerIn: parent; text: qsTr("Browse Apps"); color: "#0B2415"; font.family: Theme.fontFamily; font.pixelSize: 10; font.weight: Font.DemiBold }
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: root.selectedCategory = "All Apps" }
                        }
                    }
                    PiBoard { anchors.right: parent.right; anchors.rightMargin: 34; anchors.verticalCenter: parent.verticalCenter; width: 150; height: 94; scale: 1.08 }
                }

                Row {
                    width: parent.width
                    height: 22
                    Text {
                        width: parent.width - seeAll.width
                        text: root.selectedCategory === "Featured" ? qsTr("Popular Apps") : root.selectedCategory
                        color: Theme.text(Controller.darkTheme)
                        font.family: Theme.fontFamily
                        font.pixelSize: 13
                        font.weight: Font.Bold
                    }
                    Text {
                        id: seeAll
                        visible: root.selectedCategory === "Featured"
                        text: qsTr("See all  →")
                        color: Theme.accent
                        font.family: Theme.fontFamily
                        font.pixelSize: 9
                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: root.selectedCategory = "All Apps" }
                    }
                }

                Flow {
                    id: appFlow
                    width: parent.width
                    spacing: 9

                    Repeater {
                        model: softwareModel
                        Rectangle {
                            id: appCard
                            required property int index
                            required property string title
                            required property string subtitle
                            required property string appCategory
                            required property string iconName
                            required property color iconColor
                            required property bool installed
                            required property bool installing
                            required property bool updateAvailable
                            required property bool featured
                            readonly property bool included: root.matches(title, appCategory, installed, updateAvailable, featured)
                            visible: included
                            width: Math.floor((appFlow.width - 18) / 3)
                            height: included ? 112 : 0
                            radius: 9
                            color: cardMouse.containsMouse ? "#E52B3934" : "#DB202C28"
                            border.width: 1
                            border.color: "#34473F"

                            Row {
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.top: parent.top
                                anchors.margins: 11
                                spacing: 9
                                Rectangle {
                                    width: 36; height: 36; radius: 8; color: Qt.darker(appCard.iconColor, 1.9)
                                    border.width: 1; border.color: appCard.iconColor
                                    SystemIcon { anchors.centerIn: parent; width: 21; height: 21; name: appCard.iconName; color: appCard.iconColor }
                                }
                                Column {
                                    width: parent.width - 45
                                    spacing: 3
                                    Text { width: parent.width; text: appCard.title; elide: Text.ElideRight; color: Theme.text(Controller.darkTheme); font.family: Theme.fontFamily; font.pixelSize: 10; font.weight: Font.DemiBold }
                                    Text { width: parent.width; text: appCard.subtitle; elide: Text.ElideRight; color: Theme.muted(Controller.darkTheme); font.family: Theme.fontFamily; font.pixelSize: 8 }
                                }
                            }

                            Rectangle {
                                anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
                                anchors.leftMargin: 11; anchors.rightMargin: 11; anchors.bottomMargin: 9
                                height: 27; radius: 5
                                color: installMouse.containsMouse ? "#397D58" : (appCard.installed && !appCard.updateAvailable ? "#283B34" : "#28533E")
                                border.width: 1; border.color: appCard.installed && !appCard.updateAvailable ? "#40534A" : "#347551"
                                Text {
                                    anchors.centerIn: parent
                                    text: appCard.installing ? qsTr("Installing…") : (appCard.updateAvailable ? qsTr("Update") : (appCard.installed ? qsTr("Open") : qsTr("Install")))
                                    color: appCard.installed && !appCard.updateAvailable ? Theme.text(Controller.darkTheme) : Theme.accent
                                    font.family: Theme.fontFamily
                                    font.pixelSize: 9
                                    font.weight: Font.DemiBold
                                }
                                MouseArea {
                                    id: installMouse
                                    anchors.fill: parent
                                    enabled: !appCard.installing
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        if (appCard.installed && !appCard.updateAvailable) {
                                            root.activityText = qsTr("%1 opened").arg(appCard.title)
                                        } else {
                                            softwareModel.setProperty(appCard.index, "installing", true)
                                            installTimer.start()
                                            root.activityText = qsTr("Installing %1…").arg(appCard.title)
                                        }
                                    }
                                }
                                Timer {
                                    id: installTimer
                                    interval: 750
                                    onTriggered: {
                                        softwareModel.setProperty(appCard.index, "installing", false)
                                        softwareModel.setProperty(appCard.index, "installed", true)
                                        softwareModel.setProperty(appCard.index, "updateAvailable", false)
                                        root.activityText = qsTr("%1 is ready").arg(appCard.title)
                                    }
                                }
                            }
                            MouseArea { id: cardMouse; anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; height: 66; hoverEnabled: true; acceptedButtons: Qt.NoButton }
                        }
                    }
                }

                Text {
                    width: parent.width
                    visible: appFlow.height === 0
                    horizontalAlignment: Text.AlignHCenter
                    topPadding: 50
                    text: qsTr("No applications found")
                    color: Theme.muted(Controller.darkTheme)
                    font.family: Theme.fontFamily
                    font.pixelSize: 11
                }
            }
        }
    }

    Rectangle {
        id: footer
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 31
        color: "#ED0E1715"
        border.width: 1
        border.color: "#304B5550"
        Text {
            anchors.left: parent.left
            anchors.leftMargin: 178
            anchors.verticalCenter: parent.verticalCenter
            text: root.activityText
            color: Theme.muted(Controller.darkTheme)
            font.family: Theme.fontFamily
            font.pixelSize: 9
        }
        Row {
            anchors.right: parent.right
            anchors.rightMargin: 13
            anchors.verticalCenter: parent.verticalCenter
            spacing: 5
            Rectangle { width: 7; height: 7; radius: 4; color: Theme.accent }
            Text { text: qsTr("Flathub connected"); color: Theme.muted(Controller.darkTheme); font.family: Theme.fontFamily; font.pixelSize: 9 }
        }
    }
}
