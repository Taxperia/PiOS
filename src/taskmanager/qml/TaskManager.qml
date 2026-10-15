// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

ApplicationWindow {
    id: root

    width: Math.max(minimumWidth, Math.min(940, Screen.width * 0.8))
    height: Math.max(minimumHeight, Math.min(600, Screen.height * 0.8))
    minimumWidth: 860
    minimumHeight: 460
    x: Math.max(78, (Screen.width - width) / 2)
    y: Math.max(60, (Screen.height - height) / 2)
    visible: true
    title: qsTr("Fidan Task Manager")
    color: "transparent"
    flags: Qt.Window | Qt.FramelessWindowHint

    readonly property color accent: UiStyle.color("#63DE8A", UiStyle.dark)
    readonly property color foreground: UiStyle.color("#F2F7F4", UiStyle.dark)
    readonly property color secondary: UiStyle.color("#9FB2A8", UiStyle.dark)
    readonly property bool maximized: visibility === Window.Maximized
    readonly property real nameColumnWidth: Math.max(145, processList.width - 395)
    property string page: InitialTaskPage
    property var cpuHistory: []
    property var memoryHistory: []
    Connections {
        target: processBackend
        function onTotalsChanged() {
            root.cpuHistory = root.cpuHistory.concat([processBackend.totalCpu]).slice(-45)
            root.memoryHistory = root.memoryHistory.concat([processBackend.totalMemory]).slice(-45)
        }
    }
    property int selectedPid: -1
    property string selectedName: ""

    function endSelectedTask(force) {
        if (!processBackend.terminateProcess(selectedPid, force)) return
        selectedPid = -1
        selectedName = ""
        confirmDialog.close()
        processBackend.refresh()
    }

    function toggleMaximized() {
        if (maximized)
            showNormal()
        else
            showMaximized()
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

    component ActionButton: Rectangle {
        id: action
        property string label
        property bool enabled: true
        property bool danger: false
        signal invoked

        implicitWidth: actionText.implicitWidth + 28
        implicitHeight: 32
        radius: 6
        opacity: enabled ? 1.0 : 0.45
        color: actionMouse.containsMouse && enabled
               ? (danger ? UiStyle.color("#A94444", UiStyle.dark) : UiStyle.color("#397D58", UiStyle.dark))
               : (danger ? UiStyle.color("#753A3A", UiStyle.dark) : UiStyle.color("#28533E", UiStyle.dark))
        border.width: 1
        border.color: danger ? UiStyle.color("#A95151", UiStyle.dark) : UiStyle.color("#347551", UiStyle.dark)

        Text {
            id: actionText
            anchors.centerIn: parent
            text: action.label
            color: action.danger ? UiStyle.color("#FFE8E8", UiStyle.dark) : root.foreground
            font.family: "Inter"
            font.pixelSize: 10
            font.weight: Font.DemiBold
        }
        MouseArea {
            id: actionMouse
            anchors.fill: parent
            enabled: action.enabled
            hoverEnabled: true
            cursorShape: action.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
            onClicked: action.invoked()
        }
    }

    Timer {
        interval: 2000
        running: true
        repeat: true
        onTriggered: processBackend.refresh()
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

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "▥"
                color: root.accent
                font.family: "Inter"
                font.pixelSize: 18
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Task Manager")
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

    Rectangle {
        anchors.left: parent.left; anchors.top: titleBar.bottom; anchors.bottom: footer.top
        width: 154; color: UiStyle.color("#111C19", UiStyle.dark)
        Column {
            anchors.fill: parent; anchors.margins: 10; spacing: 8
            Repeater {
                model: [qsTr("Processes"), qsTr("Performance")]
                Rectangle {
                    required property int index
                    required property string modelData
                    width: parent.width; height: 38; radius: 7
                    color: root.page === (index === 0 ? "processes" : "performance") ? UiStyle.color("#315F46", UiStyle.dark) : "transparent"
                    Text { anchors.left: parent.left; anchors.leftMargin: 12; anchors.verticalCenter: parent.verticalCenter; text: modelData; color: root.foreground; font.family: "Inter"; font.pixelSize: 11 }
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: root.page = index === 0 ? "processes" : "performance" }
                }
            }
        }
    }
    ColumnLayout {
        visible: root.page === "processes"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: titleBar.bottom
        anchors.bottom: footer.top
        anchors.leftMargin: 174
        anchors.rightMargin: 16
        anchors.topMargin: 14
        anchors.bottomMargin: 10
        spacing: 9

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 46
            spacing: 10

            Text {
                text: qsTr("Processes")
                color: root.foreground
                font.family: "Inter"
                font.pixelSize: 20
                font.weight: Font.Bold
            }

            Rectangle {
                Layout.preferredWidth: 98
                Layout.preferredHeight: 38
                radius: 8
                color: UiStyle.color("#1A2A24", UiStyle.dark)
                border.width: 1
                border.color: UiStyle.color("#30483D", UiStyle.dark)
                Row {
                    anchors.centerIn: parent
                    spacing: 7
                    Text { text: qsTr("CPU"); color: root.secondary; font.family: "Inter"; font.pixelSize: 11 }
                    Text { text: processBackend.totalCpu.toFixed(1) + "%"; color: root.accent; font.family: "Inter"; font.pixelSize: 11; font.weight: Font.DemiBold }
                }
            }

            Rectangle {
                Layout.preferredWidth: 120
                Layout.preferredHeight: 38
                radius: 8
                color: UiStyle.color("#1A2A24", UiStyle.dark)
                border.width: 1
                border.color: UiStyle.color("#30483D", UiStyle.dark)
                Row {
                    anchors.centerIn: parent
                    spacing: 7
                    Text { text: qsTr("Memory"); color: root.secondary; font.family: "Inter"; font.pixelSize: 11 }
                    Text { text: processBackend.totalMemory.toFixed(1) + "%"; color: root.accent; font.family: "Inter"; font.pixelSize: 11; font.weight: Font.DemiBold }
                }
            }

            Item { Layout.fillWidth: true }

            TextField {
                id: processSearch
                Layout.preferredWidth: Math.min(220, Math.max(120, root.width * 0.2))
                Layout.preferredHeight: 36
                leftPadding: 13
                rightPadding: 13
                placeholderText: qsTr("Search processes…")
                placeholderTextColor: UiStyle.color("#71847A", UiStyle.dark)
                color: root.foreground
                selectByMouse: true
                font.family: "Inter"
                font.pixelSize: 10
                onTextChanged: processBackend.filter = text
                background: Rectangle {
                    radius: 7
                    color: UiStyle.color("#17221F", UiStyle.dark)
                    border.width: processSearch.activeFocus ? 1 : 0
                    border.color: root.accent
                }
            }

            ActionButton {
                label: qsTr("Refresh")
                onInvoked: processBackend.refresh()
            }
        }

        Text {
            Layout.fillWidth: true
            visible: processBackend.errorText.length > 0
            text: processBackend.errorText
            color: UiStyle.color("#FF9292", UiStyle.dark)
            font.family: "Inter"
            font.pixelSize: 10
            wrapMode: Text.Wrap
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            radius: 7
            color: UiStyle.color("#1A2823", UiStyle.dark)

            Row {
                anchors.fill: parent
                anchors.leftMargin: 12

                component HeaderCell: Text {
                    height: 36
                    verticalAlignment: Text.AlignVCenter
                    color: root.secondary
                    elide: Text.ElideRight
                    font.family: "Inter"
                    font.pixelSize: 11
                    font.weight: Font.DemiBold
                }

                HeaderCell { width: root.nameColumnWidth; text: qsTr("Name") }
                HeaderCell { width: 60; text: qsTr("PID") }
                HeaderCell { width: 90; text: qsTr("User") }
                HeaderCell { width: 70; text: qsTr("CPU") }
                HeaderCell { width: 95; text: qsTr("Memory") }
                HeaderCell { width: 55; text: qsTr("State") }
            }
        }

        ListView {
            id: processList
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 3
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            model: processBackend
            ScrollBar.vertical: ScrollBar { }

            delegate: Rectangle {
                id: processRow
                required property int pid
                required property string name
                required property string user
                required property real cpu
                required property real memory
                required property real memoryBytes
                required property string state
                required property string command

                width: processList.width
                height: 42
                radius: 7
                color: root.selectedPid === pid ? UiStyle.color("#315F46", UiStyle.dark)
                       : (rowMouse.containsMouse ? UiStyle.color("#20322B", UiStyle.dark) : "transparent")

                Row {
                    anchors.fill: parent
                    anchors.leftMargin: 12

                    component DataCell: Text {
                        height: 42
                        verticalAlignment: Text.AlignVCenter
                        color: root.foreground
                        elide: Text.ElideRight
                        font.family: "Inter"
                        font.pixelSize: 10
                    }

                    DataCell { width: root.nameColumnWidth; text: processRow.name; font.weight: Font.DemiBold }
                    DataCell { width: 60; text: processRow.pid }
                    DataCell { width: 90; text: processRow.user }
                    DataCell { width: 70; text: processRow.cpu.toFixed(1) + "%" }
                    DataCell {
                        width: 95
                        text: processRow.memoryBytes >= 1073741824
                              ? (processRow.memoryBytes / 1073741824).toFixed(1) + " GiB"
                              : (processRow.memoryBytes / 1048576).toFixed(0) + " MiB"
                    }
                    DataCell { width: 55; text: processRow.state }
                }

                MouseArea {
                    id: rowMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        root.selectedPid = processRow.pid
                        root.selectedName = processRow.name
                    }
                }

                ToolTip.visible: rowMouse.containsMouse
                ToolTip.text: processRow.command
                ToolTip.delay: 700
            }
        }
    }


    ColumnLayout {
        visible: root.page === "performance"
        anchors.left: parent.left; anchors.right: parent.right; anchors.top: titleBar.bottom; anchors.bottom: footer.top
        anchors.leftMargin: 174; anchors.rightMargin: 20; anchors.topMargin: 20; anchors.bottomMargin: 20
        spacing: 14
        Text { text: qsTr("Performance"); color: root.foreground; font.family: "Inter"; font.pixelSize: 23; font.weight: Font.Bold }
        Repeater {
            model: [ { title: "CPU", value: processBackend.totalCpu, history: root.cpuHistory }, { title: qsTr("Memory"), value: processBackend.totalMemory, history: root.memoryHistory } ]
            Rectangle {
                id: graph
                required property var modelData
                Layout.fillWidth: true; Layout.fillHeight: true; radius: 10; color: UiStyle.color("#17251F", UiStyle.dark); border.width: 1; border.color: UiStyle.color("#34473F", UiStyle.dark)
                Text { anchors.left: parent.left; anchors.top: parent.top; anchors.margins: 16; text: graph.modelData.title; color: root.foreground; font.pixelSize: 14; font.weight: Font.DemiBold }
                Text { anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 16; text: graph.modelData.value.toFixed(1) + "%"; color: root.accent; font.pixelSize: 22 }
                Canvas {
                    id: plot
                    anchors.fill: parent; anchors.leftMargin: 16; anchors.rightMargin: 16; anchors.topMargin: 55; anchors.bottomMargin: 16
                    property var points: graph.modelData.history
                    onPointsChanged: requestPaint()
                    onWidthChanged: requestPaint()
                    onHeightChanged: requestPaint()
                    onPaint: {
                        const ctx = getContext("2d"); ctx.reset(); ctx.strokeStyle = UiStyle.color("#2B4136", UiStyle.dark); ctx.lineWidth = 1
                        for (let n = 0; n <= 4; ++n) { ctx.beginPath(); ctx.moveTo(0,height*n/4); ctx.lineTo(width,height*n/4); ctx.stroke() }
                        ctx.beginPath(); ctx.strokeStyle = UiStyle.color("#63DE8A", UiStyle.dark); ctx.lineWidth = 2
                        for (let i = 0; i < points.length; ++i) {
                            const x = width * (45-points.length+i)/44; const y = height*(1-Math.max(0,Math.min(100,points[i]))/100)
                            if (i === 0) ctx.moveTo(x,y); else ctx.lineTo(x,y)
                        }
                        ctx.stroke()
                    }
                }
            }
        }
        Text { text: qsTr("Live samples every 2 seconds · %1 processes").arg(processBackend.processCount); color: root.secondary; font.pixelSize: 11 }
    }

    Rectangle {
        id: footer
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 48
        color: UiStyle.color("#101A17", UiStyle.dark)
        border.width: 1
        border.color: UiStyle.color("#2B3A35", UiStyle.dark)

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16

            Text {
                text: qsTr("%1 processes").arg(processBackend.processCount)
                color: root.secondary
                font.family: "Inter"
                font.pixelSize: 10
            }
            Item { Layout.fillWidth: true }
            Text {
                visible: root.selectedPid > 1
                text: root.selectedName
                color: root.secondary
                elide: Text.ElideRight
                font.family: "Inter"
                font.pixelSize: 10
                Layout.maximumWidth: 260
            }
            ActionButton {
                label: qsTr("End task")
                enabled: root.selectedPid > 1
                danger: true
                onInvoked: confirmDialog.open()
            }
        }
    }

    Dialog {
        id: confirmDialog
        anchors.centerIn: parent
        width: 390
        modal: true
        title: qsTr("End %1?").arg(root.selectedName)
        header: Text {
            text: confirmDialog.title; color: root.foreground
            padding: 12; font.family: "Inter"; font.pixelSize: 14; font.bold: true
        }
        standardButtons: Dialog.NoButton
        closePolicy: Popup.CloseOnEscape

        background: Rectangle {
            radius: 11
            color: UiStyle.color("#17231F", UiStyle.dark)
            border.width: 1
            border.color: UiStyle.color("#465750", UiStyle.dark)
        }

        contentItem: ColumnLayout {
            spacing: 16
            Text {
                Layout.fillWidth: true
                text: qsTr("The process will first be asked to close safely.")
                color: root.foreground
                wrapMode: Text.Wrap
                font.family: "Inter"
                font.pixelSize: 11
            }
            Text {
                Layout.fillWidth: true; visible: processBackend.errorText.length > 0
                text: processBackend.errorText; color: UiStyle.color("#FF9292", UiStyle.dark)
                wrapMode: Text.Wrap; font.pixelSize: 12
            }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                Item { Layout.fillWidth: true }
                ActionButton { label: qsTr("Cancel"); onInvoked: confirmDialog.close() }
                ActionButton {
                    label: qsTr("End task")
                    onInvoked: root.endSelectedTask(false)
                }
                ActionButton {
                    label: qsTr("Force stop")
                    danger: true
                    onInvoked: root.endSelectedTask(true)
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
}
