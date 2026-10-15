// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Layouts

SettingsControls.Card {
    contentSpacing: 0
    property bool detailed: false
    Rectangle {
        Layout.fillWidth: true; implicitHeight: 72; radius: 8; clip: true; color: "#102E25"
        Image { anchors.fill: parent; source: Product.wallpaperUrl; fillMode: Image.PreserveAspectCrop; sourceSize: Qt.size(700,180); opacity: 0.45 }
        RowLayout { anchors.fill: parent; anchors.margins: 12; spacing: 12; Image { Layout.preferredWidth: 42; Layout.preferredHeight: 42; source: "image://theme/org.fidan.Shell" } ColumnLayout { Layout.fillWidth: true; SettingsControls.Title { text: Product.name; color: "#F2F7F4"; font.pixelSize: 21 } SettingsControls.Caption { text: qsTr("Hafif, açık kaynaklı Debian masaüstü"); color: "#CADCD4" } } }
    }
    Repeater {
        model: [
            {label:qsTr("Cihaz adı"),value:SettingsData.info.host,icon:"computer-symbolic"},
            {label:qsTr("İşlemci"),value:SettingsData.info.cpu,icon:"cpu-symbolic"},
            {label:qsTr("Bellek"),value:SettingsData.info.memory,icon:"media-flash-symbolic"},
            {label:qsTr("Mimari"),value:SettingsData.info.architecture,icon:"applications-engineering-symbolic"},
            {label:qsTr("İşletim sistemi"),value:SettingsData.info.os,icon:"computer-symbolic"},
            {label:qsTr("Masaüstü"),value:Product.name+" "+Product.version,icon:"preferences-desktop-display-symbolic"},
            {label:qsTr("Paket yöneticisi"),value:"APT · Flatpak",icon:"package-x-generic-symbolic"},
            {label:qsTr("Çekirdek"),value:SettingsData.info.kernel,icon:"cpu-symbolic"},
            {label:qsTr("Ekran sunucusu"),value:SettingsData.info.session,icon:"video-display-symbolic"}
        ]
        SettingsControls.InfoRow { required property var modelData; implicitHeight:26; label: modelData.label; value: modelData.value || "—"; iconName: modelData.icon }
    }
}
