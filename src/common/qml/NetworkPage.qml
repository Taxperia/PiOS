// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root; implicitHeight: layout.implicitHeight; property string joining: ""
    readonly property var connection: SettingsData.connection
    readonly property bool wired: connection.type==="ethernet" || connection.type==="802-3-ethernet"
    onVisibleChanged: if (visible) SettingsData.refreshNetwork()
    Component.onCompleted: if (visible) SettingsData.refreshNetwork()
    ColumnLayout {
        id: layout; width: root.width; spacing: 14
        SettingsControls.Caption { text: qsTr("Kablosuz ağlara bağlan ve mevcut bağlantını yönet.") }
        GridLayout {
            Layout.fillWidth: true; columns: root.width>=760 ? 2 : 1; columnSpacing: 16; rowSpacing: 14
            ColumnLayout {
                Layout.fillWidth:true; Layout.preferredWidth:root.width*0.6; Layout.alignment:Qt.AlignTop; spacing:12
                SettingsControls.Card { RowLayout { Layout.fillWidth:true; SettingsControls.Icon { name:"network-wireless-signal-excellent-symbolic"; Layout.preferredWidth:32; Layout.preferredHeight:32 } SettingsControls.Toggle { label:qsTr("Wi-Fi"); hint:Controller.wifiAvailable ? qsTr("Kablosuz bağlantıyı aç veya kapat") : qsTr("Bu ortamda kablosuz ağ bağdaştırıcısı yok"); checked:Controller.wifiEnabled && Controller.wifiAvailable; enabled:Controller.wifiAvailable && !SettingsData.networkBusy; onToggled:{ Controller.toggleWifi(); refreshDelay.restart() } } } }
                SettingsControls.Title { text:qsTr("Mevcut ağlar") }
                SettingsControls.Card {
                    SettingsControls.Caption { visible:!SettingsData.networks.length; text:SettingsData.networkBusy ? qsTr("Ağlar aranıyor…") : !Controller.wifiAvailable ? qsTr("Wi-Fi ağı göstermek için bir kablosuz bağdaştırıcı gerekir. Kablolu bağlantı bilgilerini sağdaki panelden görebilirsin.") : !Controller.wifiEnabled ? qsTr("Ağları görmek için Wi-Fi bağlantısını aç.") : qsTr("Yakında bir kablosuz ağ bulunamadı.") }
                    Repeater {
                        model:SettingsData.networks
                        delegate: Rectangle {
                            id:network; required property var modelData; Layout.fillWidth:true; implicitHeight:66; radius:8; color:modelData.connected ? Qt.alpha(Controller.accentColor,0.14) : "transparent"; border.width:modelData.connected?1:0; border.color:Controller.accentColor
                            RowLayout { anchors.fill:parent; anchors.margins:10; spacing:12; SettingsControls.Icon { name:network.modelData.signal>70 ? "network-wireless-signal-excellent-symbolic" : network.modelData.signal>40 ? "network-wireless-signal-good-symbolic" : "network-wireless-signal-weak-symbolic" } ColumnLayout { Layout.fillWidth:true; spacing:4; SettingsControls.Label { text:network.modelData.ssid; font.pixelSize:13; elide:Text.ElideRight; maximumLineCount:1 } SettingsControls.Caption { text:(network.modelData.connected ? qsTr("Bağlı · ") : "")+network.modelData.band+" · %"+network.modelData.signal; color:network.modelData.connected ? Controller.accentColor : (Controller.darkTheme ? "#9FB6B3" : "#586B60") } } SettingsControls.Icon { visible:network.modelData.security!=="" && network.modelData.security!=="--"; name:"changes-prevent-symbolic"; Layout.preferredWidth:14; Layout.preferredHeight:14 } SettingsControls.Button { text:network.modelData.connected?qsTr("Bağlantıyı kes"):qsTr("Bağlan"); enabled:!SettingsData.networkBusy; onClicked:{ if(network.modelData.connected) SettingsData.disconnectNetwork(); else if(network.modelData.security==="" || network.modelData.security==="--") SettingsData.connectNetwork(network.modelData.ssid,""); else { root.joining=network.modelData.ssid; password.text=""; join.open() } } } }
                        }
                    }
                }
                RowLayout { Layout.fillWidth:true; SettingsControls.Button { Layout.fillWidth:true; text:SettingsData.networkBusy?qsTr("Yenileniyor…"):qsTr("Ağları yenile"); enabled:!SettingsData.networkBusy; onClicked:SettingsData.refreshNetwork(Controller.wifiEnabled) } SettingsControls.Button { text:qsTr("Bağlantıları yönet"); enabled:SettingsData.toolAvailable("network"); onClicked:SettingsData.openTool("network") } }
                SettingsControls.Caption { visible:SettingsData.networkError.length>0; text:SettingsData.networkError; color:Controller.darkTheme?"#FFB3A3":"#A43138" }
            }
            ColumnLayout {
                Layout.fillWidth:true; Layout.preferredWidth:root.width*0.4; Layout.alignment:Qt.AlignTop; spacing:12
                SettingsControls.Card {
                    RowLayout { Layout.fillWidth:true; SettingsControls.Icon { name:(root.connection.type==="802-3-ethernet" || root.connection.type==="ethernet")?"network-wired-symbolic":"network-wireless-signal-excellent-symbolic"; Layout.preferredWidth:32; Layout.preferredHeight:32 } ColumnLayout { Layout.fillWidth:true; SettingsControls.Title { text:root.connection.name || qsTr("Bağlantı bilgileri") } SettingsControls.Caption { text:root.connection.device || qsTr("Etkin bağlantı yok"); color:Controller.accentColor } } }
                    SettingsControls.InfoRow { visible:!root.wired; label:qsTr("Sinyal gücü"); value:root.connection.signal!==undefined ? "%"+root.connection.signal : "—" }
                    SettingsControls.InfoRow { visible:root.wired; label:qsTr("Bağlantı türü"); value:"Ethernet" }
                    SettingsControls.InfoRow { label:qsTr("Azami ağ hızı"); value:root.connection.rate || "—" }
                    SettingsControls.InfoRow { visible:!root.wired; label:qsTr("Frekans bandı"); value:root.connection.band || "—" }
                    SettingsControls.InfoRow { visible:!root.wired; label:qsTr("Güvenlik"); value:root.connection.security || "—" }
                    SettingsControls.InfoRow { visible:root.wired; label:qsTr("Aygıt"); value:root.connection.device || "—" }
                    SettingsControls.InfoRow { visible:root.wired; label:qsTr("MTU"); value:root.connection.mtu || "—" }
                    SettingsControls.Button { Layout.fillWidth:true; text:qsTr("Ağ ayrıntıları"); enabled:SettingsData.toolAvailable("network"); onClicked:SettingsData.openTool("network") }
                }
                SettingsControls.Card {
                    SettingsControls.Title { text:qsTr("IPv4 ayarları") }
                    SettingsControls.InfoRow { label:qsTr("IP adresi / önek"); value:root.connection.address || "—" }
                    SettingsControls.InfoRow { label:qsTr("Varsayılan ağ geçidi"); value:root.connection.gateway || "—" }
                    SettingsControls.InfoRow { label:qsTr("DNS"); value:root.connection.dns || "—" }
                    SettingsControls.Button { Layout.fillWidth:true; text:qsTr("Statik IP ve DNS ayarları"); enabled:SettingsData.toolAvailable("network"); onClicked:SettingsData.openTool("network") }
                }
            }
        }
    }
    Timer { id:refreshDelay; interval:1500; onTriggered:SettingsData.refreshNetwork() }
    Dialog {
        id:join; parent:Overlay.overlay; anchors.centerIn:parent; modal:true; width:Math.min(420,root.width-20); title:root.joining; standardButtons:Dialog.Ok | Dialog.Cancel
        ColumnLayout { width:parent.width; SettingsControls.Caption { text:qsTr("Wi-Fi parolasını gir. Kayıtlı bağlantının parolasını kullanmak için boş bırakabilirsin.") } TextField { id:password; Layout.fillWidth:true; echoMode:TextInput.Password; placeholderText:qsTr("Parola"); onAccepted:join.accept() } }
        onAccepted:{ SettingsData.connectNetwork(root.joining,password.text); password.text="" }
        onRejected:password.text=""
    }
}
