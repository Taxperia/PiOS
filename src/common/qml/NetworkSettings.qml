// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
Item {
    id: root
    property string page: "Network"
    readonly property var hub: SettingsData.desktop
    readonly property var state: hub.state
    signal navigate(string page)
    implicitHeight: body.implicitHeight
    function bytes(value) { let n=Number(value||0),units=["B","KB","MB","GB","TB"],i=0; while(n>=1024 && i<4) { n/=1024; ++i } return n.toFixed(i?1:0)+" "+units[i] }
    Component.onCompleted: { hub.refresh(); SettingsData.refreshNetwork() }
    Timer { interval: 15000; repeat: true; running: root.visible; onTriggered: hub.refresh() }
    ColumnLayout {
        id: body; width: root.width; spacing: 12
        ColumnLayout {
            visible: root.page === "Network"; Layout.fillWidth: true; spacing: 12
            SettingsControls.Card {
                RowLayout {
                    Layout.fillWidth: true; spacing: 16
                    SettingsControls.Icon { name: root.state.connected ? "network-wired-symbolic" : "network-offline-symbolic"; Layout.preferredWidth: 48; Layout.preferredHeight: 48 }
                    ColumnLayout { Layout.fillWidth: true; SettingsControls.Title { text: root.state.connectivity === "full" ? qsTr("İnternete bağlı") : root.state.connected ? qsTr("Yerel ağ bağlantısı var") : qsTr("Bağlantı yok") } SettingsControls.Caption { text: root.state.connectivity === "full" ? qsTr("NetworkManager internet erişimini doğruladı.") : root.state.connected ? qsTr("İnternet erişimi bu ortamda doğrulanmadı.") : qsTr("Aşağıdan bir bağlantı türü seç.") } }
                    SettingsControls.Button { text: hub.refreshing ? qsTr("Yenileniyor…") : qsTr("Yenile"); enabled: !hub.refreshing; onClicked: hub.refresh() }
                }
                SettingsControls.InfoRow { label: qsTr("İndirilen veri"); value: root.bytes(root.state.received) }
                SettingsControls.InfoRow { label: qsTr("Gönderilen veri"); value: root.bytes(root.state.sent) }
                SettingsControls.Caption { text: qsTr("Aygıt sayaçları · sistem/aygıt yeniden başlatılınca sıfırlanır. VPN trafiği ikinci kez sayılmaz; bu bir aylık kota ölçümü değildir.") }
            }
            SettingsControls.Link { objectName: "networkWifi"; label: "Wi-Fi"; hint: qsTr("Kablosuz ağlar, parola ve bağlantı ayrıntıları"); iconName: "network-wireless-symbolic"; onTriggered: root.navigate("Network/Wi-Fi") }
            SettingsControls.Link { objectName: "networkEthernet"; label: "Ethernet"; hint: qsTr("Kablolu bağlantılar ve IP yapılandırması"); iconName: "network-wired-symbolic"; onTriggered: root.navigate("Network/Ethernet") }
            SettingsControls.Link { label: "VPN"; hint: qsTr("VPN ve WireGuard bağlantılarını yönet"); iconName: "network-vpn-symbolic"; onTriggered: root.navigate("Network/VPN") }
            SettingsControls.Link { label: qsTr("Gelişmiş ağ ayarları"); hint: qsTr("Ağ bağdaştırıcıları, MAC, MTU ve bağlantı profilleri"); iconName: "preferences-system-network-symbolic"; onTriggered: root.navigate("Network/Advanced") }
        }
        ColumnLayout {
            visible: root.page === "Network/Ethernet" || root.page === "Network/Advanced"; Layout.fillWidth: true; spacing: 12
            SettingsControls.Caption { text: root.page === "Network/Ethernet" ? qsTr("Kablolu ağ bağdaştırıcıların ve gerçek bağlantı durumları.") : qsTr("Bu sistemde görülen fiziksel ve sanal ağ aygıtları.") }
            Repeater {
                model: (root.state.interfaces || []).filter(function(device){ return root.page !== "Network/Ethernet" || device.type === "Ethernet" })
                delegate: SettingsControls.Card {
                    required property var modelData
                    RowLayout { Layout.fillWidth: true; SettingsControls.Icon { name: "network-wired-symbolic" } SettingsControls.Title { text: modelData.name+" · "+modelData.type } SettingsControls.Caption { Layout.fillWidth: false; text: modelData.connected ? qsTr("Bağlı") : qsTr("Bağlı değil"); color: Controller.accentColor } }
                    SettingsControls.InfoRow { label: qsTr("IP adresleri"); value: modelData.address || "—" }
                    SettingsControls.InfoRow { label: "MAC"; value: modelData.mac || "—" }
                    SettingsControls.InfoRow { label: "MTU"; value: String(modelData.mtu || "—") }
                    SettingsControls.InfoRow { label: qsTr("Alınan / gönderilen"); value: root.bytes(modelData.received)+" / "+root.bytes(modelData.sent) }
                }
            }
            SettingsControls.Card { visible: !(root.state.interfaces || []).some(function(device){ return root.page === "Network/Advanced" || device.type === "Ethernet" }); SettingsControls.Caption { text: qsTr("Bu ortamda uygun bir ağ bağdaştırıcısı bulunamadı.") } }
            SettingsControls.Link { label: qsTr("IP, DNS ve bağlantı profilleri"); hint: qsTr("NetworkManager bağlantı düzenleyicisinde DHCP, statik IP, DNS ve proxy ayarları"); iconName: "preferences-system-network-symbolic"; enabled: SettingsData.toolAvailable("network"); onTriggered: SettingsData.openTool("network") }
        }
        ColumnLayout {
            visible: root.page === "Network/VPN"; Layout.fillWidth: true; spacing: 12
            SettingsControls.Caption { text: qsTr("NetworkManager'da kayıtlı VPN ve WireGuard profilleri. Bazı VPN türleri ek eklenti ister.") }
            Repeater {
                model: root.state.vpns || []
                delegate: SettingsControls.Card {
                    required property var modelData
                    RowLayout { Layout.fillWidth: true; SettingsControls.Icon { name: "network-vpn-symbolic" } ColumnLayout { Layout.fillWidth: true; SettingsControls.Title { text: modelData.name } SettingsControls.Caption { text: modelData.type+" · "+(modelData.active ? qsTr("Bağlı") : qsTr("Bağlı değil")) } } SettingsControls.Button { text: modelData.active ? qsTr("Bağlantıyı kes") : qsTr("Bağlan"); enabled: !hub.busy && !Controller.mockMode; onClicked: hub.activateConnection(modelData.uuid,!modelData.active) } }
                }
            }
            SettingsControls.Card { visible: !(root.state.vpns || []).length; SettingsControls.Caption { text: qsTr("Kayıtlı VPN profili yok.") } }
            SettingsControls.Button { text: qsTr("VPN ekle veya içe aktar…"); enabled: SettingsData.toolAvailable("network"); onClicked: SettingsData.openTool("network") }
        }
        SettingsControls.Caption { visible: hub.error.length > 0; text: hub.error; color: Controller.darkTheme ? "#FFB3A3" : "#A43138" }
    }
}
