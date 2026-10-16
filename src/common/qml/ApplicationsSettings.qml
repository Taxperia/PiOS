// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
Item {
    id: root
    property string page: "Applications"
    readonly property var hub: SettingsData.desktop
    readonly property var apps: hub.state.applications || []
    readonly property string selectedId: page.startsWith("Applications/Detail/") ? decodeURIComponent(page.slice(20)) : ""
    readonly property var selected: apps.find(function(app){return app.id === root.selectedId}) || ({})
    signal navigate(string page)
    implicitHeight: body.implicitHeight
    Component.onCompleted: hub.refresh()
    ColumnLayout {
        id: body; width: root.width; spacing: 12
        ColumnLayout {
            visible: root.page === "Applications"; Layout.fillWidth: true; spacing: 12
            SettingsControls.Link { objectName: "installedApps"; label: qsTr("Yüklü uygulamalar"); hint: qsTr("Bu kullanıcıya görünen Debian, yerel ve Flatpak uygulamaları"); value: String(root.apps.length); iconName: "view-app-grid-symbolic"; onTriggered: root.navigate("Applications/Installed") }
            SettingsControls.Link { label: qsTr("Varsayılan uygulamalar"); hint: qsTr("Dosyaları ve bağlantıları hangi uygulamanın açacağını seç"); iconName: "preferences-desktop-default-applications-symbolic"; onTriggered: root.navigate("Applications/Defaults") }
            SettingsControls.Link { label: qsTr("Uygulama mağazası"); hint: qsTr("Yeni uygulamalar kur ve mağazanın desteklediği uygulamaları yönet"); iconName: "system-software-install-symbolic"; onTriggered: Controller.openSoftwareCenter() }
        }
        ColumnLayout {
            visible: root.page === "Applications/Installed"; Layout.fillWidth: true; spacing: 10
            RowLayout {
                Layout.fillWidth: true
                TextField {
                    id: search; objectName: "installedAppsSearch"; Layout.fillWidth: true; implicitHeight: 38
                    placeholderText: qsTr("Ad veya paket kimliğiyle ara…"); color: Controller.darkTheme ? "#F2F7F4" : "#18261F"; placeholderTextColor: "#82998C"; selectByMouse: true
                    background: Rectangle { radius: 8; color: Controller.darkTheme ? "#102825" : "#F4FAF6"; border.width: 1; border.color: search.activeFocus ? Controller.accentColor : (Controller.darkTheme ? "#426053" : "#BDCEC4") }
                }
                SettingsControls.Button { text: hub.refreshing ? qsTr("Taranıyor…") : qsTr("Yenile"); enabled: !hub.refreshing; onClicked: hub.refresh() }
            }
            SettingsControls.Caption { text: qsTr("Başlatılabilen uygulamalar listelenir; kitaplık paketleri uygulama sayısına eklenmez.") }
            Repeater {
                model: root.apps.filter(function(app){return (app.name+" "+app.id+" "+(app.package || "")).toLowerCase().includes(search.text.toLowerCase())})
                delegate: SettingsControls.Link { required property var modelData; label: modelData.name; hint: modelData.source+" · "+modelData.version; value: modelData.sizeText || (modelData.size ? (modelData.size/1048576).toFixed(1)+" MB" : ""); iconName: encodeURIComponent(modelData.icon || "application-x-executable"); onTriggered: root.navigate("Applications/Detail/"+encodeURIComponent(modelData.id)) }
            }
            SettingsControls.Caption { visible: !root.apps.length; text: hub.refreshing ? qsTr("Uygulamalar okunuyor…") : qsTr("Bu kullanıcı için uygulama bulunamadı.") }
        }
        ColumnLayout {
            visible: root.page.startsWith("Applications/Detail/"); Layout.fillWidth: true; spacing: 12
            SettingsControls.Card {
                RowLayout { Layout.fillWidth: true; SettingsControls.Icon { name: encodeURIComponent(root.selected.icon || "application-x-executable"); Layout.preferredWidth: 48; Layout.preferredHeight: 48 } SettingsControls.Title { text: root.selected.name || qsTr("Uygulama artık mevcut değil") } }
                SettingsControls.InfoRow { label: qsTr("Sürüm"); value: root.selected.version || "—" }
                SettingsControls.InfoRow { label: qsTr("Kaynak"); value: root.selected.source || "—" }
                SettingsControls.InfoRow { label: qsTr("Paket / kimlik"); value: root.selected.package || root.selected.flatpak || root.selected.id || "—" }
                SettingsControls.InfoRow { label: qsTr("Kurulum kapsamı"); value: root.selected.installation || qsTr("Sistem veya yerel") }
                SettingsControls.InfoRow { label: qsTr("Paket boyutu"); value: root.selected.sizeText || (root.selected.size ? (root.selected.size/1048576).toFixed(1)+" MB" : qsTr("Raporlanmadı")) }
                SettingsControls.Caption { text: qsTr("Boyut bilgisi paket yöneticisinden gelir; kullanıcı dosyalarını ve ortak bağımlılıkları kapsamaz.") }
            }
            SettingsControls.Button { text: qsTr("Uygulama mağazasını aç"); onClicked: Controller.openSoftwareCenter() }
            SettingsControls.Link { visible: (root.selected.flatpak || "").length>0; label: qsTr("Uygulama izinleri"); hint: qsTr("Flatpak erişimlerini ve portal kamera iznini yönet"); onTriggered: root.navigate("Privacy/Permissions") }
        }
        Loader { active: root.page === "Applications/Defaults"; visible: active; Layout.fillWidth: true; Layout.preferredHeight: item ? item.implicitHeight : 0; sourceComponent: Component { OtherSettingsPage { category: "Applications" } } }
        SettingsControls.Caption { visible: hub.error.length>0; text: hub.error; color: Controller.darkTheme ? "#FFB3A3" : "#A43138" }
    }
}
