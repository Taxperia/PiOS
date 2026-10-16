// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

ApplicationWindow {
    id: root

    width: Math.max(minimumWidth, Math.min(940, Screen.width * 0.8))
    height: Math.max(minimumHeight, Math.min(620, Screen.height * 0.8))
    minimumWidth: 860
    minimumHeight: 500
    x: Math.max(78, (Screen.width - width) / 2)
    y: Math.max(60, (Screen.height - height) / 2)
    visible: true
    title: qsTr("PiOS Mağaza")
    color: "transparent"
    flags: Qt.Window | Qt.FramelessWindowHint

    readonly property color accent: UiStyle.color("#63DE8A", UiStyle.dark)
    readonly property color foreground: UiStyle.color("#F2F7F4", UiStyle.dark)
    readonly property color secondary: UiStyle.color("#9FB2A8", UiStyle.dark)
    readonly property bool maximized: visibility === Window.Maximized
    readonly property color panel: UiStyle.color("#17231F", UiStyle.dark)
    readonly property color line: UiStyle.color("#2B3A35", UiStyle.dark)
    readonly property var app: storeBackend.selectedApp
    readonly property bool detailVisible: !!app.identifier
    property string selectedCategory: "Featured"
    property string removeIdentifier: ""
    property string removeName: ""
    Shortcut { sequence: "Ctrl+F"; onActivated: searchField.forceActiveFocus() }
    Shortcut { sequence: "Alt+Left"; enabled: root.detailVisible; onActivated: storeBackend.clearSelection() }
    Shortcut { sequence: "Escape"; enabled: root.detailVisible && !removeDialog.visible; onActivated: storeBackend.clearSelection() }
    Shortcut { sequence: "F5"; enabled: !storeBackend.busy; onActivated: storeBackend.refresh() }
    onClosing: function(close) { if (storeBackend.busy) { close.accepted = false; waitDialog.open() } }


    function categoryName(value) { return ({Featured:qsTr("Keşfet"),"All Apps":qsTr("Tüm uygulamalar"),Internet:qsTr("İnternet"),Office:qsTr("Üretkenlik"),Graphics:qsTr("Tasarım"),Multimedia:qsTr("Müzik ve video"),Development:qsTr("Geliştirici araçları"),Games:qsTr("Oyunlar"),Security:qsTr("Güvenlik"),System:qsTr("Sistem"),Education:qsTr("Eğitim"),"Tools & Makers":qsTr("Araçlar ve donanım"),Installed:qsTr("Kitaplığım"),Updates:qsTr("Güncellemeler"),Downloads:qsTr("İndirmeler")})[value] || value }
    function toggleMaximized() {
        if (maximized)
            showNormal()
        else
            showMaximized()
    }

    function selectCategory(label) {
        selectedCategory = label
        storeBackend.clearSelection()
        storeBackend.installedOnly = label === "Installed" || label === "Updates"
        storeBackend.updatesOnly = label === "Updates"
        if (label === "Featured" || label === "All Apps" || label === "Installed" || label === "Updates" || label === "Downloads")
            storeBackend.category = ""
        else
            storeBackend.category = label
    }

    component Label: Text { color:root.foreground; font.family:"Inter"; font.pixelSize:13; textFormat:Text.PlainText }
    component Muted: Label { color:root.secondary; font.pixelSize:11; wrapMode:Text.Wrap }
    component Action: Button {
        id: action; property bool primary:false
        implicitHeight:38; leftPadding:18; rightPadding:18; opacity:enabled?1:0.45
        contentItem: Label { text:action.text; horizontalAlignment:Text.AlignHCenter; verticalAlignment:Text.AlignVCenter; color:action.primary?"#0C2619":root.foreground; font.weight:Font.DemiBold }
        background: Rectangle { radius:7; color:action.primary?(action.hovered?Qt.lighter(root.accent,1.1):root.accent):(action.hovered?(UiStyle.dark?"#304039":"#DBE9E0"):root.panel); border.width:action.primary?0:1; border.color:root.line }
    }
    component AppIcon: Rectangle {
        id: icon; property string iconName; property string appName:""; implicitWidth:56; implicitHeight:56; radius:width*0.23
        color:UiStyle.dark?"#315F46":"#DCE9E1"
        Image { id: art; anchors.fill:parent; anchors.margins:parent.width*0.13; source:"image://theme/"+encodeURIComponent(icon.iconName)+(UiStyle.dark?"?theme=dark":"?theme=light"); sourceSize:Qt.size(128,128); fillMode:Image.PreserveAspectFit }
    }

    component ProgressCard: Rectangle {
        property bool compact:false
        implicitHeight: progressBody.implicitHeight+26; color:root.panel; radius:12; border.color:root.line
        ColumnLayout {
            id:progressBody; x:14; y:13; width:parent.width-28; spacing:8
            RowLayout { Layout.fillWidth:true; Label { Layout.fillWidth:true; text:storeBackend.operationName || qsTr("İndirmeler"); font.weight:Font.DemiBold } Label { text:storeBackend.progress>=0?storeBackend.progress+"%":qsTr("Hazırlanıyor…"); color:root.accent; font.pixelSize:12 } }
            ProgressBar { objectName:"transaction-progress"; Layout.fillWidth:true; implicitHeight:6; from:0; to:100; value:Math.max(0,storeBackend.progress); indeterminate:storeBackend.busy && storeBackend.progress<0; background:Rectangle { radius:3; color:root.line } contentItem:Item { implicitHeight:6; Rectangle { width:parent.width*(storeBackend.progress<0?0.28:Math.max(0,storeBackend.progress)/100); height:6; radius:3; color:root.accent; SequentialAnimation on opacity { running:storeBackend.busy && storeBackend.progress<0; loops:Animation.Infinite; NumberAnimation { to:0.35; duration:650 } NumberAnimation { to:1; duration:650 } } } } }
            RowLayout { Layout.fillWidth:true; Muted { Layout.fillWidth:true; text:storeBackend.phaseText; color:storeBackend.errorText.length?"#EF9C97":root.secondary } Action { visible:storeBackend.canRetry; text:qsTr("Tekrar dene"); enabled:!storeBackend.busy; onClicked:storeBackend.retry() } Action { visible:compact; text:qsTr("Ayrıntılar"); onClicked:root.selectCategory("Downloads") } }
            Muted { visible:!compact && storeBackend.errorText.length>0; Layout.fillWidth:true; text:storeBackend.errorText; color:UiStyle.dark?"#FFB7AE":"#9F2923"; maximumLineCount:6; elide:Text.ElideRight }
        }
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
                    Layout.fillHeight: true
                    clip: true
                    ScrollBar.vertical: ScrollBar { }
                    spacing: 3
                    model: [
                        { label: "Featured", iconName: "go-home" },
                        { label: "All Apps", iconName: "view-grid" },
                        { label: "Development", iconName: "applications-development" },
                        { label: "Tools & Makers", iconName: "applications-engineering" },
                        { label: "Education", iconName: "applications-education" },
                        { label: "Multimedia", iconName: "applications-multimedia" },
                        { label: "System", iconName: "preferences-system" },
                        { label: "Internet", iconName: "applications-internet" },
                        { label: "Office", iconName: "applications-office" },
                        { label: "Graphics", iconName: "applications-graphics" },
                        { label: "Games", iconName: "applications-games" },
                        { label: "Security", iconName: "security-high" }
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

                SidebarItem { Layout.fillWidth: true; label: "Downloads"; iconName: "folder-downloads" }

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

                RowLayout {
                    Layout.fillWidth: true
                    Action { objectName: "store-back"; visible: root.detailVisible; text: "←"; onClicked: storeBackend.clearSelection() }
                TextField {
                    id: searchField
                    objectName: "store-search"
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
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 132
                    visible: !root.detailVisible && root.selectedCategory !== "Downloads" && root.selectedCategory === "Featured" && searchField.text.length === 0
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
                    visible: !root.detailVisible && root.selectedCategory !== "Downloads"
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
                    objectName: "store-grid"
                    visible: !root.detailVisible && root.selectedCategory !== "Downloads"
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
                        objectName: "app-card-" + index
                        required property int index
                        required property string name
                        required property string summary
                        required property string category
                        required property string iconName
                        required property string source
                        required property bool compatible
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
                                enabled: !storeBackend.busy && ((!appCard.installed && appCard.compatible) || appCard.launchable || root.selectedCategory === "Updates")
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
                            onClicked: storeBackend.selectEntry(appCard.index)
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
        Loader { id:detailLoader; objectName:"store-detail"; active:root.detailVisible; visible:active; Layout.fillWidth:true; Layout.fillHeight:true; sourceComponent:detailPage }
        ScrollView {
            id:downloadsScroll
            visible:root.selectedCategory==="Downloads" && !root.detailVisible; Layout.fillWidth:true; Layout.fillHeight:true; clip:true; contentWidth:availableWidth
            ColumnLayout {
                width:downloadsScroll.availableWidth; spacing:15
                Label { text:qsTr("İndirmeler ve işlemler"); font.pixelSize:24; font.weight:Font.Bold }
                Muted { Layout.fillWidth:true; text:qsTr("Yüzde o anki paket veya aşamaya aittir. Kaynak ilerleme bildirmediğinde hazırlık göstergesi kullanılır.") }
                ProgressCard { Layout.fillWidth:true; visible:storeBackend.operationIdentifier.length>0 }
                Muted { visible:!storeBackend.operationIdentifier.length; text:qsTr("Henüz bir yükleme veya kaldırma işlemi yok.") }
                Label { visible:storeBackend.operationOutput.length>0; text:qsTr("İşlem günlüğü"); font.weight:Font.DemiBold }
                TextArea { objectName:"store-operation-log"; visible:storeBackend.operationOutput.length>0; Layout.fillWidth:true; readOnly:true; selectByMouse:true; wrapMode:TextEdit.Wrap; text:storeBackend.operationOutput; color:root.secondary; font.family:"monospace"; font.pixelSize:10; background:Rectangle { radius:9; color:root.panel } }
            }
        }
            }
        }
    }

    ProgressCard {
        id: operationPanel
        compact: true
        visible: storeBackend.operationIdentifier.length > 0
        anchors.left: parent.left; anchors.leftMargin: 188
        anchors.right: parent.right; anchors.rightMargin: 14
        anchors.bottom: footer.top; anchors.bottomMargin: 10
        height: implicitHeight
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

    Component {
        id:detailPage
        ScrollView {
            id:detailScroll
            clip:true; contentWidth:availableWidth
            ColumnLayout {
                width:detailScroll.availableWidth; spacing:18
                Rectangle {
                    Layout.fillWidth:true; implicitHeight:182; radius:14; color:root.panel; border.color:root.line
                    RowLayout {
                        anchors.fill:parent; anchors.margins:22; spacing:22
                        AppIcon { iconName:root.app.iconName || "application-x-executable"; Layout.preferredWidth:100; Layout.preferredHeight:100 }
                        ColumnLayout { Layout.fillWidth:true; spacing:10
                            Label { objectName:"detail-app-name"; Layout.fillWidth:true; text:root.app.name || ""; font.pixelSize:26; font.weight:Font.Bold; wrapMode:Text.Wrap }
                            Muted { Layout.fillWidth:true; text:root.categoryName(root.app.category || "")+"  ·  "+(root.app.source==="apt"?"Debian":"Flathub"); font.pixelSize:12 }
                            RowLayout { spacing:8
                                Action { objectName:"detail-primary-action"; primary:true; text:root.app.entryBusy?qsTr("İşlem sürüyor…"):root.app.installed?(root.app.launchable?qsTr("Aç"):qsTr("Kurulu")):root.app.compatible?qsTr("Yükle"):qsTr("Bu cihazla uyumlu değil"); enabled:!storeBackend.busy && (root.app.installed?root.app.launchable:root.app.compatible); onClicked:root.app.installed?storeBackend.openSelected():storeBackend.installSelected() }
                                Action { visible:!!root.app.updateAvailable; text:qsTr("Güncelle"); enabled:!storeBackend.busy; onClicked:storeBackend.updateSelected() }
                                Action { objectName:"detail-remove"; visible:!!root.app.installed; text:qsTr("Kaldır"); enabled:!storeBackend.busy; onClicked:{root.removeIdentifier=root.app.identifier;root.removeName=root.app.name;removeDialog.open()} }
                            }
                        }
                    }
                }
                RowLayout {
                    Layout.fillWidth:true; spacing:12
                    Repeater { model:[{label:qsTr("Kaynak"),value:root.app.source==="apt"?"Debian APT":"Flathub"},{label:qsTr("İndirme"),value:root.app.downloadSize || (root.app.loading?qsTr("Alınıyor…"):"—")},{label:qsTr("Disk alanı"),value:root.app.installedSize || "—"},{label:qsTr("Mimari"),value:root.app.architecture || "—"}]
                        Rectangle { required property var modelData; Layout.fillWidth:true; implicitHeight:66; radius:9; color:root.panel; border.color:root.line; Column { anchors.fill:parent; anchors.margins:12; spacing:7; Muted { text:modelData.label; font.pixelSize:10 } Label { width:parent.width; text:modelData.value; elide:Text.ElideRight; font.pixelSize:12 } } }
                    }
                }
                Label { text:qsTr("Bu uygulama hakkında"); font.pixelSize:18; font.weight:Font.DemiBold }
                Label { Layout.fillWidth:true; text:root.app.description || root.app.summary || ""; wrapMode:Text.Wrap; lineHeight:1.35; font.pixelSize:13 }
                ColumnLayout { Layout.fillWidth:true; spacing:10; Repeater { model:root.app.features || []; RowLayout { required property string modelData; Layout.fillWidth:true; Label { text:"✓"; color:root.accent } Label { Layout.fillWidth:true; text:modelData; wrapMode:Text.Wrap; font.pixelSize:12 } } } }
                Label { text:qsTr("Paket bilgileri"); font.pixelSize:18; font.weight:Font.DemiBold; Layout.topMargin:5 }
                Muted { Layout.fillWidth:true; text:root.app.source==="flatpak"?qsTr("Uygulama hesabına kurulur. İlk yüklemede ortak çalışma ortamları için ek indirme ve disk alanı gerekebilir."):qsTr("Debian paket yöneticisiyle sisteme kurulur. Yükleme ve kaldırma yönetici yetkisi gerektirir.") }
                Muted { visible:!!root.app.error; Layout.fillWidth:true; text:root.app.error || ""; color:UiStyle.dark?"#FFB7AE":"#9F2923" }
                GridLayout {
                    Layout.fillWidth:true; columns:2; columnSpacing:22; rowSpacing:10
                    Muted { text:qsTr("Paket kimliği") } Label { Layout.fillWidth:true; text:root.app.identifier || ""; font.pixelSize:11; wrapMode:Text.WrapAnywhere }
                    Muted { text:qsTr("Sürüm / yayın") } Label { Layout.fillWidth:true; text:root.app.version || root.app.commit || (root.app.loading?qsTr("Alınıyor…"):qsTr("Kaynak belirtmiyor")); font.pixelSize:11 }
                    Muted { visible:!!root.app.branch; text:qsTr("Dağıtım dalı") } Label { visible:!!root.app.branch; text:root.app.branch || ""; font.pixelSize:11 }
                    Muted { visible:!!root.app.license; text:qsTr("Lisans") } Label { visible:!!root.app.license; text:root.app.license || ""; font.pixelSize:11 }
                }
                Action { text:qsTr("Kaynak sayfasını aç  ↗"); visible:!!root.app.website; onClicked:storeBackend.openWebsite() }
                Item { implicitHeight:8 }
            }
        }
    }
    Dialog {
        id:removeDialog; objectName:"store-remove-dialog"; parent:Overlay.overlay; anchors.centerIn:parent; width:420; padding:20; modal:true
        header:Item { implicitHeight:54; Label { x:20; y:20; text:qsTr("Uygulamayı kaldır"); font.pixelSize:16; font.weight:Font.DemiBold } }
        contentItem:Label { text:root.removeName+qsTr(" kaldırılsın mı? Paket yöneticisi kaldırma sonucunu doğrulayacak."); wrapMode:Text.Wrap }
        footer:Item { implicitHeight:65; Row { anchors.right:parent.right; anchors.rightMargin:20; y:12; spacing:8; Action { objectName:"remove-cancel"; text:qsTr("Vazgeç"); onClicked:removeDialog.reject() } Action { objectName:"remove-confirm"; text:qsTr("Kaldır"); primary:true; onClicked:removeDialog.accept() } } }
        background:Rectangle { radius:12; color:root.panel; border.color:root.line }
        onAccepted:storeBackend.removeApplication(root.removeIdentifier)
    }
    Dialog {
        id:waitDialog; parent:Overlay.overlay; anchors.centerIn:parent; width:420; padding:20; modal:true
        header:Item { implicitHeight:54; Label { x:20; y:20; text:qsTr("İşlem devam ediyor"); font.pixelSize:16; font.weight:Font.DemiBold } }
        contentItem:Label { text:qsTr("Paketlerin yarım kalmaması için işlem bitene kadar mağazayı açık tut."); wrapMode:Text.Wrap }
        footer:Item { implicitHeight:65; Action { anchors.right:parent.right; anchors.rightMargin:20; y:12; text:qsTr("Tamam"); primary:true; onClicked:waitDialog.accept() } }
        background:Rectangle { radius:12; color:root.panel; border.color:root.line }
    }
}
