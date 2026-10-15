// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    implicitHeight: layout.implicitHeight
    signal chooseWallpaper(bool lockScreen)
    readonly property string wallpaper: Controller.wallpaperUrl.length ? Controller.wallpaperUrl : Product.wallpaperUrl.toString()
    readonly property var wallpaperChoices: {
        const choices=SettingsData.wallpapers.slice()
        if (!choices.some(function(item){ return item.url.toString()===root.wallpaper })) {
            const original=root.wallpaper.includes("fidan-mountain-dawn") ? choices.findIndex(function(item){ return item.url.toString().includes("fidan-mountain-dawn") }) : -1
            const current={url:root.wallpaper,name:qsTr("Geçerli duvar kâğıdı")}
            if(original>=0) choices[original]=current
            else choices.unshift(current)
        }
        return choices.slice(0,8)
    }
    readonly property var colors: ["#63de8a", "#2589ff", "#ac55ef", "#ed449a", "#ff9546", "#ffd35d", "#ff575e", "#24c5cc"]
    ColumnLayout {
        id: layout; width: root.width; spacing: 12
        SettingsControls.Caption { text: qsTr("Masaüstünü kendi tarzına göre düzenle.") }
        GridLayout {
            Layout.fillWidth: true; columns: root.width >= 760 ? 2 : 1; columnSpacing: 12; rowSpacing: 12
            SettingsControls.Card {
                Layout.preferredWidth: root.width*0.63; Layout.alignment: Qt.AlignTop
                RowLayout { Layout.fillWidth: true; ColumnLayout { Layout.fillWidth: true; SettingsControls.Title { text: qsTr("Duvar kâğıdı") } SettingsControls.Caption { text: qsTr("Masaüstün için bir arka plan seç") } } SettingsControls.Button { text: qsTr("Fotoğraf seç…"); primary: true; onClicked: root.chooseWallpaper(false) } }
                GridLayout {
                    Layout.fillWidth: true; columns: 4; columnSpacing: 9; rowSpacing: 9
                    Repeater {
                        model: root.wallpaperChoices
                        delegate: Rectangle {
                            id: tile; required property var modelData
                            Layout.fillWidth: true; implicitHeight: Math.max(68, width*0.66); radius: 8; clip: true
                            color: "#142D2A"; border.width: 2; border.color: root.wallpaper === modelData.url.toString() ? Controller.accentColor : (Controller.darkTheme ? "#49615A" : "#BFD1C5")
                            Image { anchors.fill: parent; anchors.margins: 3; source: tile.modelData.url; sourceSize: Qt.size(300,180); fillMode: Image.PreserveAspectCrop; asynchronous: true }
                            Rectangle { visible: root.wallpaper === tile.modelData.url.toString(); anchors.right: parent.right; anchors.bottom: parent.bottom; anchors.margins: 7; width: 22; height: 22; radius: 11; color: Controller.accentColor; Text { anchors.centerIn: parent; text: "✓"; color: "#062014"; font.bold: true } }
                            MouseArea { anchors.fill: parent; hoverEnabled: true; onClicked: Controller.wallpaperUrl = tile.modelData.url.toString(); cursorShape: Qt.PointingHandCursor; ToolTip.visible: containsMouse; ToolTip.text: tile.modelData.name }
                        }
                    }
                }
            }
            SettingsControls.Card {
                Layout.preferredWidth: root.width*0.37; Layout.alignment: Qt.AlignTop
                SettingsControls.Title { text: qsTr("Canlı önizleme") }
                SettingsControls.Caption { text: qsTr("Seçimlerin masaüstünde hemen uygulanır") }
                Rectangle {
                    Layout.fillWidth: true; implicitHeight: Math.max(150,width*0.56); radius: 8; clip: true; color: "#18362C"
                    Image { anchors.fill: parent; source: root.wallpaper; fillMode: Image.PreserveAspectCrop; sourceSize: Qt.size(600,400) }
                    Rectangle {
                        id: previewDock; property bool horizontal: Controller.dockPosition === "bottom"
                        x: horizontal ? (parent.width-width)/2 : Controller.dockPosition === "right" ? parent.width-width-5 : 5
                        y: horizontal ? parent.height-height-22 : 24
                        width: horizontal ? 130 : 22; height: horizontal ? 22 : 130; radius: 5; color: "#E41A2A25"
                        Grid { anchors.centerIn: parent; columns: previewDock.horizontal ? 6 : 1; spacing: 3; Repeater { model: ["user-home-symbolic","utilities-terminal-symbolic","folder-symbolic","web-browser-symbolic","system-software-install-symbolic","preferences-system-symbolic"]; Image { required property string modelData; width: 17; height: 17; source: "image://theme/"+modelData+"?theme=dark" } } }
                    }
                    Rectangle {
                        x: parent.width*0.22; y: parent.height*0.2; width: parent.width*0.64; height: parent.height*0.56; radius: 6; color: Controller.darkTheme ? "#14231E" : "#ECF5EF"; border.width: 1; border.color: "#537163"
                        Rectangle { x: 6; y: 6; width: 5; height: 5; radius: 3; color: Controller.accentColor }
                        Row { x: 8; y: parent.height*0.4; spacing: 9; Repeater { model: 3; Rectangle { width: 30; height: 27; radius: 4; color: Qt.alpha(Controller.accentColor,0.25); Image { anchors.centerIn: parent; width: 22; height: 22; source: "image://theme/folder" } } } }
                    }
                    Rectangle { anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; height: 20; color: "#E4162B25"; Row { x: 8; spacing: 12; Text { text: "1"; color: Controller.accentColor; font.bold: true } Text { text: "2   3"; color: "white"; font.pixelSize: 10 } } Text { anchors.right: parent.right; anchors.rightMargin: 8; text: Controller.clockText; color: "white"; font.pixelSize: 10 } }
                }
            }
        }
        GridLayout {
            Layout.fillWidth: true; columns: root.width >= 760 ? 2 : 1; columnSpacing: 12; rowSpacing: 12
            SettingsControls.Card {
                SettingsControls.Title { text: qsTr("Görünüm modu") }
                SettingsControls.Caption { text: Controller.themeMode === "auto" ? qsTr("Otomatik: 08.00–19.00 açık, diğer saatlerde koyu") : qsTr("Açık, koyu veya saate göre otomatik tema") }
                RowLayout { Layout.fillWidth: true; spacing: 8; Repeater { model: [{id:"light",name:qsTr("Açık"),icon:"weather-clear-symbolic"},{id:"dark",name:qsTr("Koyu"),icon:"weather-clear-night-symbolic"},{id:"auto",name:qsTr("Otomatik"),icon:"display-brightness-symbolic"}]; SettingsControls.Option { required property var modelData; objectName: "theme-"+modelData.id; text: modelData.name; iconName: modelData.icon; selected: Controller.themeMode === modelData.id; onClicked: Controller.themeMode = modelData.id } } }
            }
            SettingsControls.Card {
                SettingsControls.Title { text: qsTr("Vurgu rengi") }
                SettingsControls.Caption { text: qsTr("Seçimler, düğmeler ve etkin öğeler için") }
                RowLayout {
                    Layout.fillWidth: true; spacing: 8
                    Repeater { model: root.colors; delegate: Rectangle { required property string modelData; Layout.fillWidth: true; Layout.maximumWidth: 42; implicitHeight: width; Layout.minimumHeight: 25; radius: width/2; color: "transparent"; border.width: Controller.accentColor === modelData ? 2 : 0; border.color: modelData; Rectangle { anchors.fill: parent; anchors.margins: 5; radius: width/2; color: modelData } MouseArea { anchors.fill: parent; onClicked: Controller.accentColor = modelData; cursorShape: Qt.PointingHandCursor } } }
                    SettingsControls.Button { text: "+"; onClicked: accentPicker.open() }
                }
            }
        }
        GridLayout {
            Layout.fillWidth: true; columns: root.width >= 760 ? 3 : 1; columnSpacing: 12; rowSpacing: 12
            SettingsControls.Card {
                SettingsControls.Title { text: qsTr("Arayüz yoğunluğu") }
                SettingsControls.Caption { text: qsTr("Panel ve masaüstü menülerinin boyutu") }
                RowLayout { Layout.fillWidth: true; spacing: 6; Repeater { model: [{name:qsTr("Kompakt"),scale:1},{name:qsTr("Varsayılan"),scale:1.25},{name:qsTr("Rahat"),scale:1.5}]; SettingsControls.Option { required property var modelData; text: modelData.name; iconName:"view-list-symbolic"; selected: Math.abs(Controller.interfaceScale-modelData.scale)<0.01; onClicked: Controller.interfaceScale = modelData.scale } } }
            }
            SettingsControls.Card {
                SettingsControls.Title { text: qsTr("Görev çubuğu") }
                SettingsControls.Caption { text: qsTr("Ekrandaki konumunu seç") }
                RowLayout { Layout.fillWidth: true; spacing: 6; Repeater { model: [{id:"left",name:qsTr("Sol")},{id:"bottom",name:qsTr("Alt")},{id:"right",name:qsTr("Sağ")}]; SettingsControls.Option { required property var modelData; text: modelData.name; dockSide: modelData.id; selected: Controller.dockPosition === modelData.id; onClicked: Controller.dockPosition = modelData.id } } }
            }
            SettingsControls.Card {
                SettingsControls.Title { text: qsTr("Simge boyutu") }
                SettingsControls.Caption { text: qsTr("Görev çubuğu simgelerini büyüt veya küçült") }
                RowLayout { Layout.fillWidth: true; SettingsControls.Slider { from: 20; to: 48; stepSize: 2; value: Controller.dockIconSize; onMoved: Controller.dockIconSize = Math.round(value) } SettingsControls.Caption { Layout.fillWidth: false; text: Controller.dockIconSize + " px" } }
                Row { Layout.alignment: Qt.AlignHCenter; spacing: 9; Repeater { model: ["user-home-symbolic","utilities-terminal-symbolic","folder","preferences-system-symbolic"]; Image { required property string modelData; width: Math.min(30, Controller.dockIconSize); height: width; source: "image://theme/"+modelData+(Controller.darkTheme?"?theme=dark":"?theme=light") } } }
            }
        }
        SettingsControls.Card {
            RowLayout {
                Layout.fillWidth: true; spacing: 12
                SettingsControls.Icon { name: "changes-prevent-symbolic" }
                ColumnLayout { Layout.fillWidth: true; SettingsControls.Title { text: qsTr("Kilit ekranı duvar kâğıdı") } SettingsControls.Caption { text: qsTr("Masaüstünden farklı bir fotoğraf kullanabilirsin") } }
                Image { Layout.preferredWidth: 90; Layout.preferredHeight: 50; source: Controller.lockWallpaperUrl.length ? Controller.lockWallpaperUrl : root.wallpaper; fillMode: Image.PreserveAspectCrop; sourceSize: Qt.size(180,100); clip: true }
                SettingsControls.Button { text: qsTr("Masaüstünü kullan"); onClicked: Controller.lockWallpaperUrl = "" }
                SettingsControls.Button { text: qsTr("Seç…"); primary: true; onClicked: root.chooseWallpaper(true) }
            }
            SettingsControls.Toggle { label: qsTr("Masaüstü simgeleri"); checked: Controller.desktopIconsVisible; onToggled: value => Controller.desktopIconsVisible = value }
            SettingsControls.Toggle { label: qsTr("Hareketi azalt"); checked: Controller.reducedMotion; onToggled: value => Controller.reducedMotion = value }
        }
    }
    Dialog {
        id: accentPicker; parent: Overlay.overlay; anchors.centerIn: parent; modal: true; width: 320; title: qsTr("Özel vurgu rengi")
        standardButtons: Dialog.Ok | Dialog.Cancel
        TextField { id: colorText; width: parent.width; text: Controller.accentColor; placeholderText: "#63de8a"; validator: RegularExpressionValidator { regularExpression: /#[0-9a-fA-F]{6}/ } }
        onAccepted: { if (colorText.acceptableInput) Controller.accentColor = colorText.text }
    }
}
