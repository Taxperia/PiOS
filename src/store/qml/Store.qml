// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

ApplicationWindow {
    id: root

    width: Math.max(minimumWidth, Math.min(940, Screen.width * 0.8))
    height: Math.max(minimumHeight, Math.min(620, Screen.height * 0.8))
    minimumWidth: 720
    minimumHeight: 500
    x: Math.max(78, (Screen.width - width) / 2)
    y: Math.max(60, (Screen.height - height) / 2)
    visible: true
    title: qsTr("Fidan Store")
    color: "transparent"
    flags: Qt.Window | Qt.FramelessWindowHint

    readonly property color accent: UiStyle.color("#63DE8A", UiStyle.dark)
    readonly property color foreground: UiStyle.color("#F2F7F4", UiStyle.dark)
    readonly property color secondary: UiStyle.color("#9FB2A8", UiStyle.dark)
    readonly property bool maximized: visibility === Window.Maximized
    property string selectedCategory: "Featured"
    property string removeIdentifier: ""
    property string removeName: ""
    Shortcut { sequence: "Ctrl+F"; onActivated: searchField.forceActiveFocus() }
    Shortcut { sequence: "F5"; enabled: !storeBackend.busy; onActivated: storeBackend.refresh() }
    onClosing: function(close) { if (storeBackend.busy) { close.accepted = false; waitDialog.open() } }


    function toggleMaximized() {
        if (maximized)
            showNormal()
        else
            showMaximized()
    }

    function selectCategory(label) {
        selectedCategory = label
        storeBackend.installedOnly = label === "Installed" || label === "Updates"
        storeBackend.updatesOnly = label === "Updates"
        if (label === "Featured" || label === "All Apps" || label === "Installed" || label === "Updates")
            storeBackend.category = ""
        else
            storeBackend.category = label
    }

    component WindowControl: Rectangle {
        id: control
        property string symbol
        property bool destructive: false
        signal invoked

        width: 46
        height: 42
        color: controlMouse.containsMouse
               ? (destructive ? UiStyle.color("#C74646", UiStyle.dark) : UiStyle.color("#304039", UiStyle.dark)) : "transparent"

        Text {
            anchors.centerIn: parent
            text: control.symbol
            color: control.destructive && controlMouse.containsMouse ? "white" : root.foreground
            font.family: "Inter"
            font.pixelSize: 14
        }

        MouseArea {
            id: controlMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: control.invoked()
        }
    }

    component SidebarItem: Rectangle {
        id: sidebarItem
        required property string label
        required property string iconName

        implicitWidth: 150
        width: parent ? parent.width : implicitWidth
        height: 36
        radius: 7
        color: root.selectedCategory === label
               ? UiStyle.color("#315F46", UiStyle.dark) : (sidebarMouse.containsMouse ? UiStyle.color("#23332D", UiStyle.dark) : "transparent")

        Row {
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            spacing: 9

            Rectangle {
                width: 18
                height: 18
                radius: 5
                color: root.selectedCategory === sidebarItem.label ? UiStyle.color("#28543D", UiStyle.dark) : UiStyle.color("#1B2924", UiStyle.dark)

                Image {
                    anchors.centerIn: parent
                    width: 14
                    height: 14
                    source: "image://theme/" + encodeURIComponent(sidebarItem.iconName) + (UiStyle.dark ? "?theme=dark" : "?theme=light")
                    sourceSize: Qt.size(18, 18)
                    fillMode: Image.PreserveAspectFit
                }
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: sidebarItem.label
                color: root.foreground
                font.family: "Inter"
                font.pixelSize: 11
                font.weight: root.selectedCategory === sidebarItem.label ? Font.DemiBold : Font.Normal
            }
        }

        MouseArea {
            id: sidebarMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: root.selectCategory(sidebarItem.label)
        }
    }

    background: Rectangle {
        radius: root.maximized ? 0 : 13
        color: UiStyle.color("#0E1715", UiStyle.dark)
        border.width: root.maximized ? 0 : 1
        border.color: UiStyle.color("#465750", UiStyle.dark)
        clip: true
    }

    Rectangle {
        id: titleBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 42
        color: UiStyle.color("#101A17", UiStyle.dark)

        MouseArea {
            anchors.left: parent.left
            anchors.right: windowControls.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            cursorShape: root.maximized ? Qt.ArrowCursor : Qt.SizeAllCursor
            onPressed: root.startSystemMove()
            onDoubleClicked: root.toggleMaximized()
        }

        Row {
            anchors.left: parent.left
            anchors.leftMargin: 14
            anchors.verticalCenter: parent.verticalCenter
            spacing: 9

            Image {
                anchors.verticalCenter: parent.verticalCenter
                width: 18
                height: 18
                source: "image://theme/system-software-install" + (UiStyle.dark ? "?theme=dark" : "?theme=light")
                sourceSize: Qt.size(22, 22)
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Software Center")
                color: root.foreground
                font.family: "Inter"
                font.pixelSize: 12
                font.weight: Font.DemiBold
            }
        }

        Row {
            id: windowControls
            anchors.right: parent.right
            anchors.top: parent.top
            height: parent.height

            WindowControl { symbol: "−"; onInvoked: root.showMinimized() }
            WindowControl { symbol: root.maximized ? "❐" : "□"; onInvoked: root.toggleMaximized() }
            WindowControl { symbol: "×"; destructive: true; onInvoked: root.close() }
        }
    }

    RowLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: titleBar.bottom
        anchors.bottom: operationPanel.visible ? operationPanel.top : footer.top
        spacing: 0

        Rectangle {
            Layout.preferredWidth: 174
            Layout.fillHeight: true
            color: UiStyle.color("#111C19", UiStyle.dark)

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 6

                Text {
                    Layout.leftMargin: 9
                    Layout.topMargin: 4
                    text: qsTr("Discover")
                    color: root.secondary
                    font.family: "Inter"
                    font.pixelSize: 11
                    font.weight: Font.DemiBold
                }

                ListView {
                    id: categoryList
                    Layout.fillWidth: true
                    Layout.preferredHeight: contentHeight
                    interactive: false
                    spacing: 3
                    model: [
                        { label: "Featured", iconName: "go-home" },
                        { label: "All Apps", iconName: "view-grid" },
                        { label: "Development", iconName: "applications-development" },
                        { label: "Tools & Makers", iconName: "applications-engineering" },
                        { label: "Education", iconName: "applications-education" },
                        { label: "Multimedia", iconName: "applications-multimedia" },
                        { label: "System", iconName: "preferences-system" }
                    ]
                    delegate: SidebarItem {
                        required property var modelData
                        label: modelData.label
                        iconName: modelData.iconName
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    Layout.topMargin: 5
                    Layout.bottomMargin: 5
                    color: UiStyle.color("#2B3A35", UiStyle.dark)
                }

                SidebarItem {
                    Layout.fillWidth: true
                    label: "Installed"
                    iconName: "folder-download"
                }

                SidebarItem { Layout.fillWidth: true; label: "Updates"; iconName: "system-software-update" }

                Item { Layout.fillHeight: true }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.margins: 8
                    spacing: 7

                    Rectangle { width: 7; height: 7; radius: 4; color: root.accent }
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("APT · Flatpak")
                        color: root.secondary
                        font.family: "Inter"
                        font.pixelSize: 11
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: UiStyle.color("#0B1412", UiStyle.dark)

            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                anchors.topMargin: 12
                anchors.bottomMargin: 10
                spacing: 10

                TextField {
                    id: searchField
                    Layout.fillWidth: true
                    Layout.preferredHeight: 36
                    leftPadding: 36
                    rightPadding: 12
                    placeholderText: qsTr("Search apps, tools, and more…")
                    placeholderTextColor: UiStyle.color("#71847A", UiStyle.dark)
                    color: root.foreground
                    selectByMouse: true
                    font.family: "Inter"
                    font.pixelSize: 11
                    onTextChanged: storeBackend.filter = text

                    background: Rectangle {
                        radius: 7
                        color: UiStyle.color("#17221F", UiStyle.dark)
                        border.width: searchField.activeFocus ? 1 : 0
                        border.color: root.accent
                    }

                    Image {
                        anchors.left: parent.left
                        anchors.leftMargin: 11
                        anchors.verticalCenter: parent.verticalCenter
                        width: 15
                        height: 15
                        source: "image://theme/edit-find" + (UiStyle.dark ? "?theme=dark" : "?theme=light")
                        sourceSize: Qt.size(18, 18)
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 132
                    visible: root.selectedCategory === "Featured" && searchField.text.length === 0
                    radius: 11
                    color: UiStyle.color("#153D2D", UiStyle.dark)
                    border.width: 1
                    border.color: UiStyle.color("#2F684D", UiStyle.dark)
                    clip: true

                    Rectangle {
                        x: parent.width * 0.64
                        y: -80
                        width: 270
                        height: 270
                        radius: 135
                        color: UiStyle.color("#194934", UiStyle.dark)
                    }
                    Rectangle {
                        x: parent.width * 0.78
                        y: -24
                        width: 180
                        height: 180
                        radius: 90
                        color: UiStyle.color("#20563E", UiStyle.dark)
                    }

                    Column {
                        anchors.left: parent.left
                        anchors.leftMargin: 20
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width * 0.55
                        spacing: 5

                        Text {
                            text: qsTr("Tools for a bigger imagination")
                            color: "white"
                            font.family: "Inter"
                            font.pixelSize: 18
                            font.weight: Font.Bold
                        }
                        Text {
                            width: parent.width
                            text: qsTr("Apps for coding, making, learning and more on Raspberry Pi.")
                            color: UiStyle.color("#D2E3DA", UiStyle.dark)
                            font.family: "Inter"
                            font.pixelSize: 10
                            wrapMode: Text.Wrap
                        }
                        Rectangle {
                            width: 102
                            height: 29
                            radius: 6
                            color: browseMouse.containsMouse ? UiStyle.color("#82E9A2", UiStyle.dark) : root.accent
                            Text {
                                anchors.centerIn: parent
                                text: qsTr("Browse Apps")
                                color: UiStyle.color("#0B2415", UiStyle.dark)
                                font.family: "Inter"
                                font.pixelSize: 10
                                font.weight: Font.DemiBold
                            }
                            MouseArea {
                                id: browseMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.selectCategory("All Apps")
                            }
                        }
                    }

                    PiBoard {
                        anchors.right: parent.right; anchors.rightMargin: 22
                        anchors.verticalCenter: parent.verticalCenter
                        width: Math.min(170, parent.width * 0.3); height: 100
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 24

                    Text {
                        text: root.selectedCategory === "Featured" ? qsTr("Popular Apps") : root.selectedCategory
                        color: root.foreground
                        font.family: "Inter"
                        font.pixelSize: 13
                        font.weight: Font.Bold
                    }
                    Item { Layout.fillWidth: true }
                    Text {
                        visible: root.selectedCategory === "Featured"
                        text: qsTr("See all  →")
                        color: root.accent
                        font.family: "Inter"
                        font.pixelSize: 10
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.selectCategory("All Apps")
                        }
                    }
                }

                GridView {
                    id: appGrid
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    model: storeBackend
                    cellWidth: width >= 480 ? width / 3 : width / 2
                    cellHeight: 116
                    ScrollBar.vertical: ScrollBar { }

                    delegate: Rectangle {
                        id: appCard
                        required property int index
                        required property string name
                        required property string summary
                        required property string category
                        required property string iconName
                        required property string source
                        required property bool launchable
                        required property string identifier
                        required property bool installed
                        required property bool entryBusy
                        required property bool updateAvailable

                        width: appGrid.cellWidth - 9
                        height: 106
                        radius: 9
                        color: cardHover.containsMouse ? UiStyle.color("#25352F", UiStyle.dark) : UiStyle.color("#1A2823", UiStyle.dark)
                        border.width: 1
                        border.color: cardHover.containsMouse ? UiStyle.color("#4A685A", UiStyle.dark) : UiStyle.color("#34473F", UiStyle.dark)

                        ToolTip {
                            id: appDetails
                            visible: cardHover.containsMouse
                            delay: 500
                            timeout: 8000
                            text: appCard.name + "\n" + appCard.summary
                            width: Math.min(320, appGrid.width - 16)
                            contentItem: Text { text: appDetails.text; textFormat: Text.PlainText; wrapMode: Text.Wrap; color: root.foreground; font.pixelSize: 12 }
                            background: Rectangle { color: UiStyle.color("#17231F", UiStyle.dark); radius: 7; border.color: UiStyle.color("#465750", UiStyle.dark) }
                        }

                        Row {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.margins: 11
                            spacing: 9

                            Rectangle {
                                width: 40
                                height: 40
                                radius: 9
                                color: UiStyle.color("#315F46", UiStyle.dark)

                                Image {
                                    id: appIcon
                                    anchors.fill: parent
                                    anchors.margins: 6
                                    source: "image://theme/" + encodeURIComponent(appCard.iconName) + (UiStyle.dark ? "?theme=dark" : "?theme=light")
                                    sourceSize: Qt.size(36, 36)
                                    fillMode: Image.PreserveAspectFit
                                    asynchronous: true
                                }
                                Text {
                                    anchors.centerIn: parent
                                    visible: appIcon.status === Image.Error
                                    text: appCard.name.charAt(0)
                                    color: root.foreground
                                    font.family: "Inter"
                                    font.pixelSize: 18
                                    font.weight: Font.Bold
                                }
                            }

                            Column {
                                width: parent.width - 49
                                spacing: 3
                                Text {
                                    width: parent.width
                                    text: appCard.name
                                    color: root.foreground
                                    elide: Text.ElideRight
                                    font.family: "Inter"
                                    font.pixelSize: 11
                                    font.weight: Font.DemiBold
                                }
                                Text {
                                    width: parent.width
                                    text: appCard.summary
                                    color: root.secondary
                                    elide: Text.ElideRight
                                    font.family: "Inter"
                                    font.pixelSize: 11
                                }
                                Text {
                                    width: parent.width
                                    text: appCard.category + "  •  " + appCard.source.toUpperCase()
                                    color: UiStyle.color("#709682", UiStyle.dark)
                                    elide: Text.ElideRight
                                    font.family: "Inter"
                                    font.pixelSize: 10
                                }
                            }
                        }

                        Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            anchors.leftMargin: 11
                            anchors.rightMargin: appCard.installed ? 85 : 11
                            anchors.bottomMargin: 9
                            height: 28
                            radius: 5
                            opacity: appCard.installed && !appCard.launchable && root.selectedCategory !== "Updates" ? 0.5 : 1
                            color: actionMouse.containsMouse ? UiStyle.color("#397D58", UiStyle.dark) : (appCard.installed ? UiStyle.color("#2A3933", UiStyle.dark) : UiStyle.color("#28533E", UiStyle.dark))
                            border.width: 1
                            border.color: appCard.installed ? UiStyle.color("#43554C", UiStyle.dark) : UiStyle.color("#347551", UiStyle.dark)

                            Text {
                                anchors.centerIn: parent
                                text: appCard.entryBusy ? qsTr("Working…") : root.selectedCategory === "Updates" ? qsTr("Update") : appCard.installed ? qsTr("Open") : qsTr("Install")
                                color: appCard.entryBusy || appCard.installed ? root.foreground : root.accent
                                font.family: "Inter"
                                font.pixelSize: 11
                                font.weight: Font.DemiBold
                            }
                            MouseArea {
                                id: actionMouse
                                anchors.fill: parent
                                enabled: !storeBackend.busy && (!appCard.installed || appCard.launchable || root.selectedCategory === "Updates")
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.selectedCategory === "Updates" ? storeBackend.updateEntry(appCard.index) : appCard.installed ? storeBackend.openEntry(appCard.index) : storeBackend.installOrRemove(appCard.index)
                            }
                        }

                        Rectangle {
                            visible: appCard.installed; width: 66; height: 28
                            anchors.right: parent.right; anchors.rightMargin: 11; anchors.bottom: parent.bottom; anchors.bottomMargin: 9
                            radius: 5; color: removeMouse.containsMouse ? UiStyle.color("#304039", UiStyle.dark) : UiStyle.color("#2A3933", UiStyle.dark)
                            border.width: 1; border.color: UiStyle.color("#43554C", UiStyle.dark)
                            Text { anchors.centerIn: parent; text: qsTr("Remove"); color: root.foreground; font.pixelSize: 11 }
                            MouseArea { id: removeMouse; anchors.fill: parent; hoverEnabled: true; enabled: !storeBackend.busy; onClicked: { root.removeIdentifier = appCard.identifier; root.removeName = appCard.name; removeDialog.open() } }
                        }
                        MouseArea {
                            id: cardHover
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            height: 72
                            hoverEnabled: true
                            acceptedButtons: Qt.NoButton
                        }
                    }

                    Text {
                        anchors.centerIn: parent
                        visible: appGrid.count === 0
                        text: root.selectedCategory === "Updates" ? qsTr("No catalog updates available") : qsTr("No applications found")
                        color: root.secondary
                        font.family: "Inter"
                        font.pixelSize: 12
                    }
                }
            }
        }
    }

    Rectangle {
        id: operationPanel
        visible: storeBackend.busy || storeBackend.errorText.length > 0
        anchors.left: parent.left; anchors.leftMargin: 188
        anchors.right: parent.right; anchors.rightMargin: 14
        anchors.bottom: footer.top; anchors.bottomMargin: 10
        height: 144; radius: 9; color: UiStyle.color("#17231F", UiStyle.dark)
        border.width: 1; border.color: storeBackend.errorText.length ? UiStyle.color("#B85D5D", UiStyle.dark) : UiStyle.color("#465750", UiStyle.dark)
        ColumnLayout {
            anchors.fill: parent; anchors.margins: 12; spacing: 6
            Text {
                Layout.fillWidth: true
                text: storeBackend.errorText.length ? qsTr("Operation could not be completed") : qsTr("Package operation in progress")
                color: storeBackend.errorText.length ? UiStyle.color("#FF9292", UiStyle.dark) : root.foreground
                font.family: "Inter"; font.pixelSize: 12; font.bold: true
            }
            ProgressBar {
                id: packageProgress
                Layout.fillWidth: true; visible: storeBackend.busy
                from: 0; to: 100; value: Math.max(0, storeBackend.progress); indeterminate: storeBackend.progress < 0
                background: Rectangle { implicitHeight: 5; radius: 3; color: UiStyle.color("#34473F", UiStyle.dark) }
                contentItem: Item {
                    implicitHeight: 5
                    Rectangle {
                        width: parent.width * (packageProgress.indeterminate ? 0.4 : packageProgress.visualPosition)
                        x: packageProgress.indeterminate ? parent.width * 0.3 : 0
                        height: parent.height; radius: 3; color: root.accent
                        SequentialAnimation on opacity {
                            running: packageProgress.indeterminate && storeBackend.busy; loops: Animation.Infinite
                            NumberAnimation { to: 0.35; duration: 600 }
                            NumberAnimation { to: 1; duration: 600 }
                        }
                    }
                }
            }
            Text {
                visible: !storeBackend.busy && storeBackend.canRetry; text: qsTr("Retry operation"); color: root.accent; font.pixelSize: 12
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: storeBackend.retry() }
            }
            ScrollView {
                Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                TextArea {
                    objectName: "packageOperationDetails"
                    text: storeBackend.errorText.length ? storeBackend.errorText
                        : storeBackend.operationOutput.length ? storeBackend.operationOutput : storeBackend.statusText
                    readOnly: true; selectByMouse: true; wrapMode: TextEdit.Wrap
                    color: root.foreground; font.family: "Inter"; font.pixelSize: 11
                    background: null
                    onTextChanged: if (storeBackend.busy) cursorPosition = length
                }
            }
        }
    }

    Rectangle {
        id: footer
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 34
        color: UiStyle.color("#101A17", UiStyle.dark)
        border.width: 1
        border.color: UiStyle.color("#2B3A35", UiStyle.dark)

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 184
            anchors.rightMargin: 12
            spacing: 10

            Text {
                Layout.fillWidth: true
                text: storeBackend.errorText.length > 0 ? storeBackend.errorText : storeBackend.statusText
                color: storeBackend.errorText.length > 0 ? UiStyle.color("#FF9292", UiStyle.dark) : root.secondary
                elide: Text.ElideRight
                font.family: "Inter"
                font.pixelSize: 11
            }
            Text {
                text: storeBackend.busy ? qsTr("Working…") : qsTr("Refresh")
                color: storeBackend.busy ? root.secondary : root.accent
                font.family: "Inter"
                font.pixelSize: 11
                MouseArea {
                    anchors.fill: parent
                    enabled: !storeBackend.busy
                    cursorShape: Qt.PointingHandCursor
                    onClicked: storeBackend.refresh()
                }
            }
        }
    }

    MouseArea {
        anchors.left: parent.left
        anchors.top: titleBar.bottom
        anchors.bottom: footer.top
        width: 6
        visible: !root.maximized
        cursorShape: Qt.SizeHorCursor
        onPressed: root.startSystemResize(Qt.LeftEdge)
    }
    MouseArea {
        anchors.right: parent.right
        anchors.top: titleBar.bottom
        anchors.bottom: footer.top
        width: 6
        visible: !root.maximized
        cursorShape: Qt.SizeHorCursor
        onPressed: root.startSystemResize(Qt.RightEdge)
    }
    MouseArea {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 6
        visible: !root.maximized
        cursorShape: Qt.SizeVerCursor
        onPressed: root.startSystemResize(Qt.BottomEdge)
    }

    component DialogButton: Button {
        id: button; font.pixelSize: 12
        contentItem: Text { text: button.text; color: root.foreground; font: button.font; horizontalAlignment: Text.AlignHCenter }
        background: Rectangle { radius: 7; color: button.hovered ? UiStyle.color("#397D58", UiStyle.dark) : UiStyle.color("#28533E", UiStyle.dark) }
    }
    Dialog {
        id: removeDialog; objectName: "removeApplicationDialog"; anchors.centerIn: parent; width: 380; modal: true; header: null
        background: Rectangle { radius: 11; color: UiStyle.color("#17231F", UiStyle.dark); border.width: 1; border.color: UiStyle.color("#465750", UiStyle.dark) }
        contentItem: ColumnLayout {
            spacing: 14
            Text { text: qsTr("Remove %1?").arg(root.removeName); color: root.foreground; font.pixelSize: 16; font.bold: true }
            Text { Layout.fillWidth: true; text: qsTr("The application will be uninstalled. Personal application data will be kept."); color: root.secondary; wrapMode: Text.Wrap; font.pixelSize: 12 }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                DialogButton { text: qsTr("Cancel"); onClicked: removeDialog.close() }
                DialogButton { text: qsTr("Remove"); onClicked: { storeBackend.removeApplication(root.removeIdentifier); removeDialog.close() } }
            }
        }
    }
    Dialog {
        id: waitDialog; anchors.centerIn: parent; width: 360; modal: true; header: null
        background: Rectangle { radius: 11; color: UiStyle.color("#17231F", UiStyle.dark) }
        contentItem: ColumnLayout {
            Text { Layout.fillWidth: true; text: qsTr("A package operation is running. Wait for it to finish before closing the store."); color: root.foreground; wrapMode: Text.Wrap }
            DialogButton { text: qsTr("OK"); onClicked: waitDialog.close() }
        }
    }
}
