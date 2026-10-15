// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Window

Item {
    id: root
    implicitHeight: layout.implicitHeight
    property color foreground: Controller.darkTheme ? "#F2F7F4" : "#18261F"
    property color secondary: Controller.darkTheme ? "#AEBBB5" : "#586B60"
    property color borderColor: Controller.darkTheme ? "#40594D" : "#BCCFC2"
    readonly property color accent: Controller.darkTheme ? "#63DE8A" : "#207243"
    readonly property var output: Displays.currentOutput
    property int draftWidth: 0
    property int draftHeight: 0
    property real draftRefresh: 0
    property real draftScale: 1
    property string draftTransform: "normal"
    signal chooseWallpaper()
    readonly property var resolutions: {
        let result = []
        for (const mode of output.modes || []) {
            if (result.some(function(value) { return value.width === mode.width && value.height === mode.height })) continue
            result.push({width: mode.width, height: mode.height, label: mode.width + " × " + mode.height + (mode.preferred ? qsTr(" (Recommended)") : "")})
        }
        return result
    }
    readonly property var rates: (output.modes || []).filter(function(mode) { return mode.width === root.draftWidth && mode.height === root.draftHeight })
    readonly property var scales: [0.75, 1, 1.25, 1.5, 1.75, 2]
    readonly property bool dirty: {
        const mode = currentMode(output)
        return draftWidth !== (mode.width || 0) || draftHeight !== (mode.height || 0) || Math.abs(draftRefresh - (mode.refresh || 0)) > 0.01 || Math.abs(draftScale - (output.scale || 1)) > 0.01 || draftTransform !== (output.transform || "normal")
    }
    function currentMode(item) { return (item.modes || []).find(function(mode) { return mode.current }) || (item.modes || [])[0] || ({}) }
    function resetDraft() {
        const mode = currentMode(output)
        draftWidth = mode.width || 0; draftHeight = mode.height || 0
        draftRefresh = mode.refresh || 0; draftScale = output.scale || 1; draftTransform = output.transform || "normal"
    }
    Component.onCompleted: resetDraft()
    onVisibleChanged: if (visible) Displays.refresh()
    Connections {
        target: Displays
        function onSelectionChanged() { root.resetDraft() }
        function onConfirmationChanged() { Displays.confirmationPending ? confirmation.open() : confirmation.close() }
    }
    component Panel: Rectangle {
        default property alias contents: body.data
        Layout.fillWidth: true
        implicitHeight: body.implicitHeight + 24
        radius: 10; color: Controller.darkTheme ? "#C1182521" : "#EBF9FCFA"
        border.width: 1; border.color: root.borderColor
        ColumnLayout { id: body; x: 12; y: 12; width: parent.width - 24; spacing: 9 }
    }
    component Caption: Text {
        color: root.secondary; font.family: "Inter"; font.pixelSize: 11; wrapMode: Text.Wrap
        Layout.fillWidth: true
    }
    component Heading: Text {
        color: root.foreground; font.family: "Inter"; font.pixelSize: 14; font.weight: Font.DemiBold
        Layout.fillWidth: true
    }
    component Divider: Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: root.borderColor; opacity: 0.5 }
    component Action: Button {
        id: action; property bool primary: false
        font.family: "Inter"; font.pixelSize: 11; implicitHeight: 33; horizontalPadding: 14
        opacity: enabled ? 1 : 0.4
        contentItem: Text { text: action.text; color: root.foreground; font: action.font; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
        background: Rectangle { radius: 7; color: action.primary ? (Controller.darkTheme ? "#28533E" : "#C7E5D3") : (Controller.darkTheme ? "#25332E" : "#DFEAE3"); border.width: 1; border.color: action.primary ? "#347551" : root.borderColor; opacity: action.hovered ? 1 : 0.85 }
    }
    component Choice: ComboBox {
        id: choice; implicitHeight: 34; font.family: "Inter"; font.pixelSize: 11
        leftPadding: 10; rightPadding: 26; opacity: enabled ? 1 : 0.45
        contentItem: Text { text: choice.displayText; color: root.foreground; font: choice.font; verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight }
        indicator: Text { anchors.right: parent.right; anchors.rightMargin: 11; anchors.verticalCenter: parent.verticalCenter; text: "⌄"; color: root.secondary; font.pixelSize: 16 }
        background: Rectangle { radius: 7; color: Controller.darkTheme ? "#5423372D" : "#F5FAF7"; border.width: 1; border.color: choice.activeFocus ? root.accent : root.borderColor }
        delegate: ItemDelegate {
            width: choice.width; height: 36; highlighted: choice.highlightedIndex === index
            contentItem: Text { text: modelData; color: root.foreground; font: choice.font; verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight }
            background: Rectangle { color: highlighted ? (Controller.darkTheme ? "#315F46" : "#C7E5D3") : "transparent"; radius: 5 }
        }
        popup: Popup {
            y: choice.height + 4; width: choice.width; padding: 4
            implicitHeight: Math.min(240, contentItem.implicitHeight + 8)
            background: Rectangle { color: Controller.darkTheme ? "#17231F" : "#F5FAF7"; radius: 8; border.width: 1; border.color: root.borderColor }
            contentItem: ListView { clip: true; implicitHeight: contentHeight; model: choice.popup.visible ? choice.delegateModel : null; currentIndex: choice.highlightedIndex; ScrollIndicator.vertical: ScrollIndicator {} }
        }
    }
    component Level: Slider {
        id: slider; implicitHeight: 25; Layout.fillWidth: true; opacity: enabled ? 1 : 0.4
        background: Rectangle {
            x: slider.leftPadding; y: slider.topPadding + slider.availableHeight / 2 - 3
            width: slider.availableWidth; height: 6; radius: 3; color: Controller.darkTheme ? "#3C4C45" : "#C2D6C9"
            Rectangle { width: slider.visualPosition * parent.width; height: parent.height; radius: 3; color: "#54DA81" }
        }
        handle: Rectangle { width: 17; height: 17; radius: 9; x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width); y: slider.topPadding + slider.availableHeight / 2 - height / 2; color: "#74DEA1"; border.width: 1; border.color: "#D4F8DD" }
    }
    component SettingRow: RowLayout {
        property string label
        property string hint
        default property alias controls: controlArea.data
        Layout.fillWidth: true; implicitHeight: 39; spacing: 12
        ColumnLayout {
            Layout.fillWidth: true; Layout.minimumWidth: 100; spacing: 3
            Text { text: parent.parent.label; color: root.foreground; font.family: "Inter"; font.pixelSize: 12 }
            Caption { text: parent.parent.hint; font.pixelSize: 10 }
        }
        Item { id: controlArea; Layout.preferredWidth: Math.max(175, root.width * 0.26); Layout.preferredHeight: 34 }
    }
    ColumnLayout {
        id: layout; width: root.width; spacing: 12
        Caption { text: qsTr("Configure your monitors, resolution, scaling, and visual settings") }
        Rectangle {
            Layout.fillWidth: true; implicitHeight: 170; radius: 10
            color: Controller.darkTheme ? "#80101D18" : "#CFF5FAF7"; border.width: 1; border.color: root.borderColor
            Flickable {
                id: monitors; anchors.fill: parent; anchors.margins: 14; anchors.rightMargin: root.width > 650 ? 150 : 14
                contentWidth: Math.max(width, Displays.outputs.length * 208 - 16); contentHeight: height; clip: true
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.horizontal: ScrollBar {}
                Row {
                    x: Math.max(0, (monitors.width - Displays.outputs.length * 208 + 16) / 2); spacing: 16
                    Repeater {
                        model: Displays.outputs
                        delegate: Column {
                            id: thumbnail; required property var modelData
                            width: 192; spacing: 5
                            Rectangle {
                                width: 192; height: 106; radius: 7; clip: true
                                border.width: Displays.selectedName === thumbnail.modelData.name ? 2 : 1
                                border.color: Displays.selectedName === thumbnail.modelData.name ? "#63DE8A" : root.borderColor
                                color: "#17231F"
                                Image { anchors.fill: parent; anchors.margins: 3; source: Controller.wallpaperUrl.length ? Controller.wallpaperUrl : Product.wallpaperUrl; fillMode: Image.PreserveAspectCrop; sourceSize: Qt.size(400, 240) }
                                Rectangle {
                                    anchors.horizontalCenter: parent.horizontalCenter; y: 9; width: 29; height: 29; radius: 15
                                    color: Displays.selectedName === thumbnail.modelData.name ? "#D52B6842" : "#C14B5650"; border.width: 1; border.color: "#10291C"
                                    Text { anchors.centerIn: parent; text: thumbnail.modelData.number; color: "white"; font.pixelSize: 14; font.bold: true }
                                }
                                MouseArea { anchors.fill: parent; enabled: !Displays.busy && !Displays.confirmationPending; onClicked: Displays.selectedName = thumbnail.modelData.name; cursorShape: Qt.PointingHandCursor }
                            }
                            Text { width: parent.width; text: modelData.label; color: root.foreground; font.family: "Inter"; font.pixelSize: 11; horizontalAlignment: Text.AlignHCenter }
                            Text { width: parent.width; text: { const mode = root.currentMode(modelData); return (mode.width || "—") + " × " + (mode.height || "—") } color: root.secondary; font.family: "Inter"; font.pixelSize: 10; horizontalAlignment: Text.AlignHCenter }
                        }
                    }
                }
            }
            Action { anchors.right: parent.right; anchors.bottom: parent.bottom; anchors.margins: 12; visible: root.width > 650; text: qsTr("Identify Displays"); onClicked: Displays.identify() }
        }
        Caption { visible: Displays.errorText.length > 0; text: Displays.errorText; color: Controller.darkTheme ? "#FFAD9E" : "#A32D39" }
        GridLayout {
            Layout.fillWidth: true; columns: root.width >= 760 ? 2 : 1; columnSpacing: 12; rowSpacing: 12
            ColumnLayout {
                Layout.fillWidth: true; Layout.preferredWidth: root.width * 0.57; Layout.alignment: Qt.AlignTop; spacing: 12
                Panel {
                    Heading { text: qsTr("Display Settings") }
                    Divider {}
                    SettingRow {
                        label: qsTr("Display"); hint: qsTr("Choose the monitor to configure")
                        Choice {
                            anchors.fill: parent; objectName: "displaySelector"
                            model: Displays.outputs.map(function(output) { return output.number + ". " + output.label + (output.primary ? qsTr(" (Primary)") : "") })
                            currentIndex: Displays.outputs.findIndex(function(output) { return output.name === Displays.selectedName })
                            enabled: !Displays.busy && !Displays.confirmationPending && !Displays.colorBusy
                            onActivated: Displays.selectedName = Displays.outputs[currentIndex].name
                        }
                    }
                    Divider {}
                    SettingRow {
                        label: qsTr("Resolution"); hint: qsTr("Number of pixels on screen")
                        Choice {
                            anchors.fill: parent; objectName: "displayResolution"
                            model: root.resolutions.map(function(mode) { return mode.label })
                            currentIndex: root.resolutions.findIndex(function(mode) { return mode.width === root.draftWidth && mode.height === root.draftHeight })
                            enabled: Displays.available && !Displays.busy && !Displays.confirmationPending
                            onActivated: {
                                const mode = root.resolutions[currentIndex]; root.draftWidth = mode.width; root.draftHeight = mode.height
                                root.draftRefresh = root.rates.length ? root.rates[0].refresh : 0
                            }
                        }
                    }
                    Divider {}
                    SettingRow {
                        label: qsTr("Refresh Rate"); hint: qsTr("How often the screen updates")
                        Choice {
                            anchors.fill: parent; objectName: "displayRefreshRate"
                            model: root.rates.map(function(mode) { return mode.refresh > 0 ? Number(mode.refresh.toFixed(2)) + " Hz" : qsTr("Automatic") })
                            currentIndex: root.rates.findIndex(function(mode) { return Math.abs(mode.refresh - root.draftRefresh) < 0.01 })
                            enabled: Displays.available && !Displays.busy && !Displays.confirmationPending
                            onActivated: root.draftRefresh = root.rates[currentIndex].refresh
                        }
                    }
                    Divider {}
                    SettingRow {
                        label: qsTr("Scaling"); hint: qsTr("Make text and items larger or smaller")
                        Choice {
                            anchors.fill: parent; objectName: "displayScale"
                            model: root.scales.map(function(scale) { return Math.round(scale * 100) + "%" + (scale === 1 ? qsTr(" (Default)") : "") })
                            currentIndex: root.scales.findIndex(function(scale) { return Math.abs(scale - root.draftScale) < 0.01 })
                            enabled: Displays.available && !Displays.busy && !Displays.confirmationPending
                            onActivated: root.draftScale = root.scales[currentIndex]
                        }
                    }
                    Divider {}
                    RowLayout {
                        Layout.fillWidth: true; spacing: 8
                        ColumnLayout { Layout.fillWidth: true; Heading { text: qsTr("Orientation"); font.pixelSize: 12 } Caption { text: qsTr("Screen rotation"); font.pixelSize: 10 } }
                        Repeater {
                            model: [{label: qsTr("Landscape"), value: "normal", angle: 0}, {label: qsTr("Portrait"), value: "90", angle: 90}, {label: qsTr("Left"), value: "270", angle: -90}, {label: qsTr("Inverted"), value: "180", angle: 180}]
                            delegate: Rectangle {
                                required property var modelData
                                Layout.preferredWidth: Math.max(56, (root.width * 0.57 - 130) / 4); implicitHeight: 57; radius: 7
                                color: root.draftTransform === modelData.value ? (Controller.darkTheme ? "#28533E" : "#C7E5D3") : (Controller.darkTheme ? "#26332D" : "#E2ECE6")
                                border.width: 1; border.color: root.draftTransform === modelData.value ? "#45BA6B" : root.borderColor
                                opacity: Displays.available ? 1 : 0.4
                                Column {
                                    anchors.centerIn: parent; spacing: 7
                                    Image { anchors.horizontalCenter: parent.horizontalCenter; width: 17; height: 17; source: "image://theme/video-display-symbolic" + (Controller.darkTheme ? "?theme=dark" : "?theme=light"); rotation: modelData.angle }
                                    Text { text: modelData.label; color: root.foreground; font.family: "Inter"; font.pixelSize: 9 }
                                }
                                MouseArea { anchors.fill: parent; enabled: Displays.available && !Displays.busy && !Displays.confirmationPending; onClicked: root.draftTransform = modelData.value; cursorShape: Qt.PointingHandCursor }
                            }
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true; visible: root.dirty
                        Caption { text: qsTr("Changes revert automatically unless confirmed.") }
                        Action { text: qsTr("Reset"); enabled: !Displays.busy; onClicked: root.resetDraft() }
                        Action { objectName: "displayApply"; text: Displays.busy ? qsTr("Applying…") : qsTr("Apply"); primary: true; enabled: Displays.available && !Displays.busy && !Displays.confirmationPending; onClicked: Displays.apply(root.draftWidth, root.draftHeight, root.draftRefresh, root.draftScale, root.draftTransform) }
                    }
                }
                Panel {
                    RowLayout {
                        Layout.fillWidth: true; spacing: 10
                        Image { Layout.preferredWidth: 25; Layout.preferredHeight: 25; sourceSize: Qt.size(32, 32); source: "image://theme/preferences-system" + (Controller.darkTheme ? "?theme=dark" : "?theme=light") }
                        ColumnLayout { Layout.fillWidth: true; spacing: 3; Heading { text: qsTr("Advanced Display Settings"); font.pixelSize: 12 } Caption { text: qsTr("Monitor information and additional display controls"); font.pixelSize: 10 } }
                        Action { text: "›"; onClicked: advanced.open() }
                    }
                }
            }
            ColumnLayout {
                Layout.fillWidth: true; Layout.preferredWidth: root.width * 0.43; Layout.alignment: Qt.AlignTop; spacing: 12
                Panel {
                    Heading { text: qsTr("Brightness and Color") }
                    Divider {}
                    RowLayout {
                        Layout.fillWidth: true; spacing: 12
                        Image { Layout.preferredWidth: 22; Layout.preferredHeight: 22; source: "image://theme/display-brightness-symbolic" + (Controller.darkTheme ? "?theme=dark" : "?theme=light") }
                        ColumnLayout {
                            Layout.fillWidth: true; spacing: 4
                            Caption { text: qsTr("Brightness"); color: root.foreground }
                            RowLayout {
                                Layout.fillWidth: true
                                Level { enabled: Controller.brightnessAvailable; from: 1; to: 100; value: Controller.brightness; onMoved: Controller.brightness = value }
                                Caption { Layout.fillWidth: false; text: Controller.brightnessAvailable ? Controller.brightness + "%" : "—" }
                            }
                        }
                    }
                    Caption { visible: !Controller.brightnessAvailable; text: qsTr("This display has no controllable backlight."); font.pixelSize: 10 }
                    Divider {}
                    RowLayout {
                        Layout.fillWidth: true; spacing: 12
                        Image { Layout.preferredWidth: 22; Layout.preferredHeight: 22; source: "image://theme/weather-clear-night-symbolic" + (Controller.darkTheme ? "?theme=dark" : "?theme=light") }
                        ColumnLayout { Layout.fillWidth: true; spacing: 4; Caption { text: qsTr("Night Light"); color: root.foreground } Caption { text: Displays.nightLightAvailable ? qsTr("Reduce blue light for warmer colors") : qsTr("Unavailable on this output"); font.pixelSize: 10 } }
                        Switch {
                            id: nightSwitch; checked: Displays.nightLightEnabled; enabled: Displays.nightLightAvailable && !Displays.colorBusy
                            onToggled: Displays.setNightLight(checked, Displays.temperature)
                            indicator: Rectangle {
                                implicitWidth: 36; implicitHeight: 21; radius: 11; x: nightSwitch.leftPadding; y: nightSwitch.topPadding + nightSwitch.availableHeight / 2 - height / 2
                                color: nightSwitch.checked ? "#43C773" : (Controller.darkTheme ? "#42514A" : "#BFD1C5"); opacity: nightSwitch.enabled ? 1 : 0.4
                                Rectangle { width: 17; height: 17; radius: 9; x: nightSwitch.checked ? 17 : 2; y: 2; color: "#EDF8F0" }
                            }
                        }
                    }
                    Divider {}
                    RowLayout {
                        Layout.fillWidth: true; spacing: 12
                        Image { Layout.preferredWidth: 22; Layout.preferredHeight: 22; source: "image://theme/sensors-temperature-symbolic" + (Controller.darkTheme ? "?theme=dark" : "?theme=light") }
                        ColumnLayout {
                            Layout.fillWidth: true; spacing: 4
                            Caption { text: qsTr("Color Temperature"); color: root.foreground }
                            RowLayout {
                                Layout.fillWidth: true
                                Level {
                                    id: temperature; enabled: Displays.nightLightAvailable && Displays.nightLightEnabled && !Displays.colorBusy
                                    from: 2500; to: 6500; stepSize: 100; value: Displays.temperature
                                    onMoved: colorDebounce.restart()
                                    background: Rectangle {
                                        x: temperature.leftPadding; y: temperature.topPadding + temperature.availableHeight / 2 - 3; width: temperature.availableWidth; height: 6; radius: 3
                                        gradient: Gradient { orientation: Gradient.Horizontal; GradientStop { position: 0; color: "#FF9E5C" } GradientStop { position: 0.5; color: "#F7F0CA" } GradientStop { position: 1; color: "#A7DBF9" } }
                                    }
                                }
                                Caption { Layout.fillWidth: false; text: (Displays.nightLightEnabled ? Math.round(temperature.value) : 6500) + "K" }
                            }
                        }
                    }
                }
                Panel {
                    Heading { text: qsTr("Wallpaper Preview") }
                    Divider {}
                    RowLayout {
                        Layout.fillWidth: true; spacing: 12
                        Image { Layout.preferredWidth: Math.min(156, root.width * 0.17); Layout.preferredHeight: 90; source: Controller.wallpaperUrl.length ? Controller.wallpaperUrl : Product.wallpaperUrl; fillMode: Image.PreserveAspectCrop; clip: true; sourceSize: Qt.size(320, 180) }
                        ColumnLayout {
                            Layout.fillWidth: true; spacing: 7
                            Caption { text: qsTr("Current Wallpaper"); font.pixelSize: 10 }
                            Caption { text: Controller.wallpaperUrl.length ? decodeURIComponent(Controller.wallpaperUrl.split("/").pop()) : qsTr("Mountain dawn"); maximumLineCount: 2; elide: Text.ElideRight }
                            Action { Layout.fillWidth: true; text: qsTr("Choose Wallpaper…"); primary: true; onClicked: root.chooseWallpaper() }
                        }
                    }
                }
            }
        }
    }
    Timer { id: colorDebounce; interval: 350; onTriggered: Displays.setNightLight(true, Math.round(temperature.value)) }
    Dialog {
        id: confirmation; objectName: "displayConfirmation"; parent: Overlay.overlay; anchors.centerIn: parent; width: 410; modal: true; header: null; closePolicy: Popup.CloseOnEscape
        onRejected: Displays.revert()
        background: Rectangle { radius: 12; color: Controller.darkTheme ? "#17231F" : "#F5FAF7"; border.width: 1; border.color: root.borderColor }
        contentItem: ColumnLayout {
            spacing: 14
            Heading { text: qsTr("Keep these display settings?") }
            Caption { text: qsTr("The previous settings will be restored in %1 seconds.").arg(Displays.secondsRemaining) }
            RowLayout { Layout.alignment: Qt.AlignRight; Action { text: qsTr("Revert"); onClicked: Displays.revert() } Action { text: qsTr("Keep Changes"); primary: true; onClicked: Displays.keep() } }
        }
    }
    Dialog {
        id: advanced; parent: Overlay.overlay; anchors.centerIn: parent; width: 460; modal: true; header: null
        background: Rectangle { radius: 12; color: Controller.darkTheme ? "#17231F" : "#F5FAF7"; border.width: 1; border.color: root.borderColor }
        contentItem: ColumnLayout {
            spacing: 12
            Heading { text: qsTr("Advanced Display Settings") }
            Caption { text: (root.output.description || root.output.label || "") + "\n" + (root.output.name || "") }
            Caption { text: qsTr("Desktop panels and menus"); color: root.foreground }
            Choice { Layout.fillWidth: true; model: ["100%", "125%", "150%"]; currentIndex: Math.round((Controller.interfaceScale - 1) * 4); onActivated: Controller.interfaceScale = 1 + currentIndex * 0.25 }
            Caption { text: qsTr("Shell interface size is separate from the monitor's output scale.") }
            Action { text: qsTr("Open additional display controls"); onClicked: Controller.openSystemTool("display") }
            Action { Layout.alignment: Qt.AlignRight; text: qsTr("Close"); onClicked: advanced.close() }
        }
    }
    Instantiator {
        model: Qt.application.screens
        delegate: Window {
            required property var modelData
            screen: modelData
            width: 170; height: 128; x: screen.virtualX + (screen.width - width) / 2; y: screen.virtualY + (screen.height - height) / 2
            visible: Displays.identifying; flags: Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.WindowTransparentForInput
            color: "#E5183526"
            Text { anchors.centerIn: parent; text: { const output = Displays.outputs.find(function(item) { return item.name === modelData.name }); return output ? output.number : "1" } color: "#63DE8A"; font.pixelSize: 62; font.bold: true }
        }
    }
}
