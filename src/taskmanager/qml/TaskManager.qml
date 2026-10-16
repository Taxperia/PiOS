// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

ApplicationWindow {
    id: root
    width: Math.min(1100, Screen.width - 110); height: Math.min(710, Screen.height - 110)
    minimumWidth: 880; minimumHeight: 540
    x: Math.max(78, (Screen.width - width) / 2); y: Math.max(40, (Screen.height - height) / 2)
    visible: true; title: qsTr("PiOS Görev Yöneticisi"); color: "transparent"
    flags: Qt.Window | Qt.FramelessWindowHint
    font.family: "Inter"
    readonly property bool maximized: visibility === Window.Maximized
    readonly property color bg: UiStyle.dark ? "#101917" : "#f2f6f3"
    readonly property color panel: UiStyle.dark ? "#17231f" : "#ffffff"
    readonly property color line: UiStyle.dark ? "#2b3a34" : "#dce5df"
    readonly property color ink: UiStyle.dark ? "#edf5ef" : "#192b21"
    readonly property color muted: UiStyle.dark ? "#95aaa0" : "#65796d"
    readonly property color accent: UiStyle.color("#63DE8A", UiStyle.dark)
    readonly property color blue: UiStyle.dark ? "#79b7ff" : "#246cc0"
    property string page: InitialTaskPage
    property bool paused: false
    property int selectedPid: -1
    property var selection: ({})
    property var cpuHistory: [processBackend.totalCpu]
    property var memoryHistory: [processBackend.totalMemory]
    readonly property real nameWidth: Math.max(160, table.width - 390)

    function bytes(value) { return value >= 1073741824 ? (value / 1073741824).toFixed(1) + " GiB" : (value / 1048576).toFixed(1) + " MiB" }
    function duration(seconds) { return Math.floor(seconds / 86400) + " gün " + Math.floor(seconds % 86400 / 3600) + " sa " + Math.floor(seconds % 3600 / 60) + " dk" }
    function stateName(state) { return ({"R":"Çalışıyor", "S":"Bekliyor", "D":"Disk bekliyor", "T":"Durduruldu", "t":"İzleniyor", "Z":"Sonlandı", "I":"Boşta"})[state] || state }
    function selectProcess(pid) { selectedPid = pid; selection = processBackend.details(pid) }
    function toggleMaximized() { if (maximized) showNormal(); else showMaximized() }
    function endTask(force) {
        if (!processBackend.terminateProcess(selectedPid, force)) return
        confirmDialog.close(); selectedPid = -1; selection = ({}); processBackend.refresh()
    }
    Connections {
        target: processBackend
        function onTotalsChanged() {
            root.cpuHistory = root.cpuHistory.concat([processBackend.totalCpu]).slice(-45)
            root.memoryHistory = root.memoryHistory.concat([processBackend.totalMemory]).slice(-45)
            if (root.selectedPid > 0) {
                root.selection = processBackend.details(root.selectedPid)
                if (!root.selection.pid) { root.selectedPid = -1; confirmDialog.close() }
            }
        }
    }
    Timer { interval: 2000; running: !root.paused; repeat: true; onTriggered: processBackend.refresh() }
    Shortcut { sequence: "Ctrl+F"; onActivated: { root.page = "processes"; search.forceActiveFocus() } }
    Shortcut { sequence: "F5"; onActivated: processBackend.refresh() }
    Shortcut { sequence: "Escape"; enabled: !confirmDialog.opened; onActivated: { root.selectedPid = -1; root.selection = ({}) } }

    component Label: Text { color: root.ink; font.family: "Inter"; font.pixelSize: 12; elide: Text.ElideRight }
    component Icon: Image {
        property string name
        source: "image://theme/" + name + "?theme=" + (UiStyle.dark ? "dark" : "light")
        sourceSize.width: 32; sourceSize.height: 32; fillMode: Image.PreserveAspectFit
    }
    component Action: Button {
        id: action
        property bool primary: false
        property bool danger: false
        implicitHeight: 34; implicitWidth: contentItem.implicitWidth + 26
        hoverEnabled: true
        background: Rectangle {
            radius: 7
            color: !action.enabled ? root.line : action.danger ? (UiStyle.dark ? "#4a292c" : "#ffe8e9") : action.primary ? root.accent : action.hovered ? (UiStyle.dark ? "#2b3d33" : "#e5eee8") : root.panel
            border.color: action.danger ? (UiStyle.dark ? "#744148" : "#efb7bd") : action.primary ? root.accent : root.line
        }
        contentItem: Text {
            text: action.text; color: !action.enabled ? root.muted : action.danger ? (UiStyle.dark ? "#ffafb7" : "#ab3040") : action.primary ? (UiStyle.dark ? "#102519" : "white") : root.ink
            font.family: "Inter"; font.pixelSize: 11; font.weight: Font.Medium
            horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
        }
    }
    component WindowControl: Rectangle {
        id: control
        property string symbol
        property bool destructive: false
        signal invoked()
        width: 44; height: 42
        color: mouse.containsMouse ? (destructive ? "#bd3e4b" : root.line) : "transparent"
        Label { anchors.centerIn: parent; text: control.symbol; font.pixelSize: 17; color: control.destructive && mouse.containsMouse ? "white" : root.ink }
        MouseArea { id: mouse; anchors.fill: parent; hoverEnabled: true; onClicked: control.invoked() }
    }
    component Metric: Rectangle {
        id: metric
        property string label
        property string value
        property string detail
        property string icon
        property real percent: -1
        property color tint: root.accent
        Layout.fillWidth: true; Layout.preferredHeight: 100
        color: root.panel; radius: 12; border.color: root.line
        Icon { x: 16; y: 15; width: 18; height: 18; name: metric.icon }
        Label { x: 42; y: 17; text: metric.label; color: root.muted; font.pixelSize: 11 }
        Label { x: 16; y: 38; text: metric.value; color: metric.tint; font.pixelSize: 27; font.weight: Font.DemiBold }
        Label { anchors.right: parent.right; anchors.rightMargin: 15; y: 51; width: parent.width - 130; text: metric.detail; horizontalAlignment: Text.AlignRight; font.pixelSize: 10; color: root.muted }
        Rectangle {
            x: 16; y: 82; width: parent.width - 32; height: 3; radius: 2; color: root.line; visible: metric.percent >= 0
            Rectangle { width: parent.width * Math.min(100, metric.percent) / 100; height: 3; radius: 2; color: metric.tint }
        }
    }
    component SortHeader: Item {
        id: heading
        property string label
        property string column
        property bool numeric: false
        height: 42
        Label {
            anchors.fill: parent; anchors.rightMargin: 12; verticalAlignment: Text.AlignVCenter
            horizontalAlignment: heading.numeric ? Text.AlignRight : Text.AlignLeft
            text: heading.label + (processBackend.sortColumn === heading.column ? (processBackend.sortDescending ? "  ↓" : "  ↑") : "")
            font.pixelSize: 10; font.weight: Font.DemiBold
            color: processBackend.sortColumn === heading.column ? root.accent : root.muted
        }
        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: processBackend.sortBy(heading.column) }
    }
    component Chart: Rectangle {
        id: graph
        property string label
        property string subtitle
        property real value
        property var points
        property color tint
        Layout.fillWidth: true; Layout.fillHeight: true; Layout.minimumHeight: 170
        color: root.panel; radius: 12; border.color: root.line
        Label { x: 18; y: 16; text: graph.label; font.pixelSize: 15; font.weight: Font.DemiBold }
        Label { x: 18; y: 40; width: parent.width - 130; text: graph.subtitle; color: root.muted; font.pixelSize: 10 }
        Label { anchors.right: parent.right; anchors.rightMargin: 18; y: 18; text: graph.value.toFixed(1) + "%"; color: graph.tint; font.pixelSize: 28; font.weight: Font.DemiBold }
        Label { anchors.right: parent.right; anchors.rightMargin: 18; y: 65; text: "100%"; color: root.muted; font.pixelSize: 9 }
        Canvas {
            id: plot
            anchors.fill: parent; anchors.leftMargin: 18; anchors.rightMargin: 18; anchors.topMargin: 82; anchors.bottomMargin: 33
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()
            Connections { target: graph; function onPointsChanged() { plot.requestPaint() } function onTintChanged() { plot.requestPaint() } }
            Connections { target: UiStyle; function onChanged() { plot.requestPaint() } }
            onPaint: {
                const ctx = getContext("2d"); ctx.reset(); ctx.lineWidth = 1; ctx.strokeStyle = root.line
                for (let n = 0; n <= 4; ++n) { ctx.beginPath(); ctx.moveTo(0, height*n/4); ctx.lineTo(width,height*n/4); ctx.stroke() }
                for (let n = 0; n <= 8; ++n) { ctx.beginPath(); ctx.moveTo(width*n/8,0); ctx.lineTo(width*n/8,height); ctx.stroke() }
                if (!graph.points.length) return
                let startX = 0
                ctx.beginPath()
                for (let i=0; i<graph.points.length; ++i) {
                    const x = width * (45-graph.points.length+i)/44
                    const y = height * (1 - Math.max(0, Math.min(100,graph.points[i]))/100)
                    if (i===0) { ctx.moveTo(x,y); startX=x } else ctx.lineTo(x,y)
                }
                ctx.strokeStyle = graph.tint; ctx.lineWidth = 2; ctx.stroke()
                ctx.lineTo(width,height); ctx.lineTo(startX,height); ctx.closePath()
                const fill = ctx.createLinearGradient(0,0,0,height); fill.addColorStop(0,Qt.rgba(graph.tint.r,graph.tint.g,graph.tint.b,.28)); fill.addColorStop(1,Qt.rgba(graph.tint.r,graph.tint.g,graph.tint.b,.02)); ctx.fillStyle=fill; ctx.fill()
            }
        }
        Label { x: 18; anchors.bottom: parent.bottom; anchors.bottomMargin: 13; text: "Son 45 ölçüm · 2 sn aralıkla"; color: root.muted; font.pixelSize: 9 }
        Label { anchors.right: parent.right; anchors.rightMargin: 18; anchors.bottom: parent.bottom; anchors.bottomMargin: 13; text: "Şimdi"; color: root.muted; font.pixelSize: 9 }
    }

    background: Rectangle { color: root.bg; radius: root.maximized ? 0 : 12; border.color: root.line }
    Item {
        id: titleBar; width: parent.width; height: 42
        MouseArea { anchors.fill: parent; anchors.rightMargin: 132; onPressed: root.startSystemMove(); onDoubleClicked: root.toggleMaximized() }
        Icon { x: 16; anchors.verticalCenter: parent.verticalCenter; width: 19; height: 19; name: "utilities-system-monitor" }
        Label { x: 45; anchors.verticalCenter: parent.verticalCenter; text: "Görev Yöneticisi"; font.pixelSize: 11; font.weight: Font.Medium }
        Row {
            anchors.right: parent.right
            WindowControl { symbol: "−"; onInvoked: root.showMinimized() }
            WindowControl { symbol: root.maximized ? "❐" : "□"; onInvoked: root.toggleMaximized() }
            WindowControl { symbol: "×"; destructive: true; onInvoked: root.close() }
        }
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: root.line }
    }
    Rectangle {
        id: sidebar; width: 170; anchors.top: titleBar.bottom; anchors.bottom: parent.bottom; color: root.panel
        Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: root.line }
        Column {
            x: 12; y: 22; width: parent.width - 24; spacing: 7
            Label { x: 10; text: "SİSTEMİ İZLE"; color: root.muted; font.pixelSize: 9; font.letterSpacing: 1.2; height: 26 }
            Repeater {
                model: [{key:"processes",label:"İşlemler",icon:"view-list-details-symbolic"}, {key:"performance",label:"Performans",icon:"utilities-system-monitor-symbolic"}]
                Rectangle {
                    id: nav; required property var modelData
                    objectName: "nav-" + modelData.key
                    width: 146; height: 42; radius: 8
                    color: root.page === modelData.key ? Qt.rgba(root.accent.r,root.accent.g,root.accent.b,.14) : navMouse.containsMouse ? root.line : "transparent"
                    Rectangle { x: 0; y: 12; width: 3; height: 18; radius: 2; color: root.accent; visible: root.page === nav.modelData.key }
                    Icon { x: 13; y: 12; width: 18; height: 18; name: nav.modelData.icon }
                    Label { x: 42; anchors.verticalCenter: parent.verticalCenter; text: nav.modelData.label; font.weight: root.page === nav.modelData.key ? Font.DemiBold : Font.Normal }
                    MouseArea { id: navMouse; anchors.fill: parent; hoverEnabled: true; onClicked: root.page = nav.modelData.key }
                }
            }
        }
        Column {
            x: 22; width: parent.width-42; anchors.bottom: parent.bottom; anchors.bottomMargin: 25; spacing: 8
            Row { spacing: 7; Rectangle { width: 6; height: 6; radius: 3; y: 4; color: root.paused ? root.muted : root.accent } Label { text: root.paused ? "İzleme duraklatıldı" : "Canlı sistem verileri"; font.pixelSize: 9; color: root.muted } }
            Label { text: "Her 2 saniyede yenilenir"; font.pixelSize: 9; color: root.muted }
            Label { text: "PiOS"; font.pixelSize: 17; font.weight: Font.Bold; color: root.accent }
        }
    }
    ColumnLayout {
        anchors.left: sidebar.right; anchors.right: parent.right; anchors.top: titleBar.bottom; anchors.bottom: parent.bottom
        anchors.margins: 22; spacing: 16
        RowLayout {
            Layout.fillWidth: true
            ColumnLayout {
                spacing: 5
                Label { text: root.page === "processes" ? "İşlemler" : "Performans"; font.pixelSize: 25; font.weight: Font.DemiBold }
                Label { text: root.page === "processes" ? "Çalışan işlemleri ve kaynak kullanımını yönetin." : "Sisteminizin kaynak kullanımını canlı takip edin."; color: root.muted; font.pixelSize: 11 }
            }
            Item { Layout.fillWidth: true }
            Action { objectName: "pause-action"; text: root.paused ? "▶  Devam et" : "Ⅱ  Duraklat"; onClicked: root.paused = !root.paused }
            Action { text: "↻  Yenile"; onClicked: processBackend.refresh(); ToolTip.visible: hovered; ToolTip.text: "F5" }
        }
        RowLayout {
            Layout.fillWidth: true; spacing: 12
            Metric { label: "İŞLEMCİ"; value: processBackend.totalCpu.toFixed(1) + "%"; detail: processBackend.cpuCount + " mantıksal çekirdek"; icon: "cpu-symbolic"; percent: processBackend.totalCpu }
            Metric { label: "BELLEK"; value: processBackend.totalMemory.toFixed(1) + "%"; detail: root.bytes(processBackend.memoryUsed) + " / " + root.bytes(processBackend.memoryCapacity); icon: "media-flash-symbolic"; percent: processBackend.totalMemory; tint: root.blue }
            Metric { label: "İŞLEMLER"; value: processBackend.processCount; detail: "Çalışma süresi\n" + root.duration(processBackend.uptime); icon: "view-list-details-symbolic"; tint: root.ink }
        }
        ColumnLayout {
            visible: root.page === "processes"; Layout.fillWidth: true; Layout.fillHeight: true; spacing: 12
            RowLayout {
                Layout.fillWidth: true
                TextField {
                    id: search; objectName: "process-search"
                    Layout.fillWidth: true; Layout.preferredHeight: 36
                    color: root.ink; placeholderTextColor: root.muted; placeholderText: "İşlem adı, kullanıcı veya PID ara…"; selectByMouse: true
                    font.pixelSize: 11; leftPadding: 13
                    onTextChanged: processBackend.filter = text
                    background: Rectangle { radius: 8; color: root.panel; border.color: search.activeFocus ? root.accent : root.line }
                }
                Action { text: "Temizle"; visible: search.text.length > 0; onClicked: search.clear() }
                Action { objectName: "end-task"; text: "Görevi sonlandır"; danger: true; enabled: root.selectedPid > 1; onClicked: confirmDialog.open() }
            }
            Rectangle {
                id: table; Layout.fillWidth: true; Layout.fillHeight: true; color: root.panel; radius: 10; border.color: root.line; clip: true
                Row {
                    id: headings; x: 14; width: parent.width-28; height: 42
                    SortHeader { width: root.nameWidth; label: "AD"; column: "name" }
                    SortHeader { width: 62; label: "PID"; column: "pid" }
                    SortHeader { width: 86; label: "KULLANICI"; column: "user" }
                    SortHeader { objectName: "sort-cpu"; width: 70; label: "CPU"; column: "cpu"; numeric: true }
                    SortHeader { objectName: "sort-memory"; width: 88; label: "BELLEK"; column: "memory"; numeric: true }
                    Label { width: 56; height: 42; text: "DURUM"; font.pixelSize: 9; color: root.muted; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight }
                }
                Rectangle { y: 42; width: parent.width; height: 1; color: root.line }
                ListView {
                    id: processList; objectName: "process-list"; anchors.fill: parent; anchors.topMargin: 44; anchors.bottomMargin: 4
                    clip: true; boundsBehavior: Flickable.StopAtBounds; model: processBackend
                    ScrollBar.vertical: ScrollBar { }
                    delegate: Rectangle {
                        id: row
                        required property int pid; required property string name; required property string user
                        required property real cpu; required property real memory; required property real memoryBytes
                        required property string state; required property string command
                        objectName: "process-row-" + pid
                        width: processList.width; height: 45
                        color: root.selectedPid === pid ? Qt.rgba(root.accent.r,root.accent.g,root.accent.b,.13) : rowMouse.containsMouse ? (UiStyle.dark ? "#203028" : "#edf3ef") : "transparent"
                        Rectangle { width: 3; height: 24; y: 10; radius: 2; color: root.accent; visible: root.selectedPid === row.pid }
                        Row {
                            x: 14; height: parent.height
                            Item {
                                width: root.nameWidth; height: parent.height
                                Rectangle { x: 0; y: 9; width: 27; height: 27; radius: 7; color: root.line
                                    Icon { anchors.centerIn: parent; width: 19; height: 19; name: row.name.startsWith("fidan-") ? "utilities-system-monitor" : row.name } }
                                Label { x: 37; width: parent.width-45; height: parent.height; verticalAlignment: Text.AlignVCenter; text: row.name; font.pixelSize: 11; font.weight: Font.Medium }
                            }
                            Label { width: 62; height: parent.height; verticalAlignment: Text.AlignVCenter; text: row.pid; color: root.muted; font.pixelSize: 10 }
                            Label { width: 86; height: parent.height; verticalAlignment: Text.AlignVCenter; text: row.user; color: root.muted; font.pixelSize: 10 }
                            Item { width: 70; height: parent.height
                                Rectangle { anchors.right: parent.right; anchors.rightMargin: 4; y: 6; height: 33; width: 64 * Math.min(row.cpu / 100,1); radius: 4; color: Qt.rgba(root.accent.r,root.accent.g,root.accent.b,.12) }
                                Label { anchors.fill: parent; anchors.rightMargin: 12; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: row.cpu.toFixed(1) + "%"; font.pixelSize: 10; color: row.cpu > 0 ? root.accent : root.muted }
                            }
                            Item { width: 88; height: parent.height
                                Rectangle { anchors.right: parent.right; anchors.rightMargin: 4; y: 6; height: 33; width: 82 * Math.min(row.memory / 100,1); radius: 4; color: Qt.rgba(root.blue.r,root.blue.g,root.blue.b,.10) }
                                Label { anchors.fill: parent; anchors.rightMargin: 12; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: root.bytes(row.memoryBytes); font.pixelSize: 10 }
                            }
                            Label { width: 56; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: row.state === "R" ? "Aktif" : row.state === "S" ? "Bekliyor" : row.state === "I" ? "Boşta" : row.state; color: row.state === "R" ? root.accent : root.muted; font.pixelSize: 9 }
                        }
                        MouseArea { id: rowMouse; anchors.fill: parent; hoverEnabled: true; onClicked: root.selectProcess(row.pid) }
                        ToolTip.visible: rowMouse.containsMouse; ToolTip.delay: 900; ToolTip.text: root.stateName(row.state) + "\n" + row.command
                    }
                    Label { anchors.centerIn: parent; visible: processList.count === 0; text: "Aramanızla eşleşen işlem yok."; color: root.muted }
                }
            }
            RowLayout {
                Layout.fillWidth: true; spacing: 10
                Label { text: root.selectedPid > 0 ? root.selection.name + "  ·  PID " + root.selectedPid : "Ayrıntıları görmek için bir işlem seçin."; color: root.selectedPid > 0 ? root.ink : root.muted; font.pixelSize: 10; Layout.maximumWidth: 280 }
                Label { Layout.fillWidth: true; text: root.selection.command || ""; color: root.muted; font.pixelSize: 9; ToolTip.visible: commandHover.hovered; ToolTip.text: text; HoverHandler { id: commandHover } }
                Label { text: processList.count + " sonuç"; color: root.muted; font.pixelSize: 10 }
            }
        }
        ColumnLayout {
            visible: root.page === "performance"; Layout.fillWidth: true; Layout.fillHeight: true; spacing: 12
            RowLayout {
                Layout.fillWidth: true; Layout.fillHeight: true; spacing: 12
                Chart { label: "İşlemci"; subtitle: processBackend.cpuName; value: processBackend.totalCpu; points: root.cpuHistory; tint: root.accent }
                Chart { label: "Bellek"; subtitle: root.bytes(processBackend.memoryUsed) + " kullanımda"; value: processBackend.totalMemory; points: root.memoryHistory; tint: root.blue }
            }
            Rectangle {
                Layout.fillWidth: true; Layout.preferredHeight: 54; radius: 9; color: root.panel; border.color: root.line
                RowLayout { anchors.fill: parent; anchors.margins: 15; spacing: 16
                    Label { text: "Kullanılabilir bellek"; color: root.muted; font.pixelSize: 10 }
                    Label { text: root.bytes(processBackend.memoryCapacity-processBackend.memoryUsed); font.weight: Font.DemiBold }
                    Item { Layout.fillWidth: true }
                    Label { text: "CPU işlem değerlerinde %100 = bir mantıksal çekirdek"; color: root.muted; font.pixelSize: 9 }
                }
            }
        }
        Label { visible: processBackend.errorText.length > 0; Layout.fillWidth: true; text: processBackend.errorText; color: UiStyle.dark ? "#ffafb7" : "#ab3040"; wrapMode: Text.Wrap }
    }
    Dialog {
        id: confirmDialog; objectName: "end-confirmation"; anchors.centerIn: parent; width: 460; modal: true; padding: 22
        closePolicy: Popup.CloseOnEscape
        background: Rectangle { radius: 13; color: root.panel; border.color: root.line }
        header: Item { }
        contentItem: ColumnLayout {
            spacing: 16
            Label { text: "Görev sonlandırılsın mı?"; font.pixelSize: 20; font.weight: Font.DemiBold }
            Label { Layout.fillWidth: true; text: (root.selection.name || "") + " · PID " + root.selectedPid; color: root.accent }
            Label { Layout.fillWidth: true; wrapMode: Text.Wrap; elide: Text.ElideNone; text: "Önce işlemin normal şekilde kapanması istenir. Kaydedilmemiş çalışmalar kaybolabilir. Zorla durdurmayı yalnızca yanıt vermeyen işlemlerde kullanın."; color: root.muted; font.pixelSize: 12 }
            Label { Layout.fillWidth: true; visible: processBackend.errorText.length > 0; wrapMode: Text.Wrap; text: processBackend.errorText; color: "#df6f7f" }
            RowLayout {
                Layout.fillWidth: true
                Action { text: "Vazgeç"; onClicked: confirmDialog.close() }
                Item { Layout.fillWidth: true }
                Action { text: "Zorla durdur"; danger: true; onClicked: root.endTask(true) }
                Action { text: "Sonlandır"; primary: true; onClicked: root.endTask(false) }
            }
        }
    }
    MouseArea { anchors.left: parent.left; anchors.top: titleBar.bottom; anchors.bottom: parent.bottom; width: 5; visible: !root.maximized; cursorShape: Qt.SizeHorCursor; onPressed: root.startSystemResize(Qt.LeftEdge) }
    MouseArea { anchors.right: parent.right; anchors.top: titleBar.bottom; anchors.bottom: parent.bottom; width: 5; visible: !root.maximized; cursorShape: Qt.SizeHorCursor; onPressed: root.startSystemResize(Qt.RightEdge) }
    MouseArea { anchors.bottom: parent.bottom; width: parent.width; height: 5; visible: !root.maximized; cursorShape: Qt.SizeVerCursor; onPressed: root.startSystemResize(Qt.BottomEdge) }
}
