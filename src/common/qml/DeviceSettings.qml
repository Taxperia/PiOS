// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
Item {
    id: root
    property string page: "Sound"
    readonly property var hub: SettingsData.desktop
    readonly property var state: hub.state
    implicitHeight: body.implicitHeight
    Component.onCompleted: hub.refresh()
    Timer { interval: 15000; repeat: true; running: root.visible; onTriggered: hub.refresh() }
    component AudioDevice: SettingsControls.Card {
        id: device
        property var record
        property string kind
        RowLayout { Layout.fillWidth: true; SettingsControls.Icon { name: device.kind === "sink" ? "audio-speakers-symbolic" : "audio-input-microphone-symbolic" } ColumnLayout { Layout.fillWidth: true; SettingsControls.Title { text: device.record.description } SettingsControls.Caption { text: device.record.default ? qsTr("Varsayılan aygıt") : device.record.name } } SettingsControls.Button { visible: !device.record.default; text: qsTr("Varsayılan yap"); enabled: !root.hub.busy && !Controller.mockMode; onClicked: root.hub.setAudioDefault(device.kind,device.record.name) } }
        RowLayout {
            Layout.fillWidth: true
            SettingsControls.Slider { id: volume; from: 0; to: 100; value: device.record.volume; enabled: !root.hub.busy && !Controller.mockMode; onMoved: if(!pressed) root.hub.setAudioVolume(device.kind,device.record.name,Math.round(value)); onPressedChanged: if(!pressed && !root.hub.busy) root.hub.setAudioVolume(device.kind,device.record.name,Math.round(value)) }
            SettingsControls.Label { Layout.fillWidth: false; text: Math.round(volume.value)+"%" }
            SettingsControls.Button { text: device.record.muted ? qsTr("Sesi aç") : qsTr("Sessize al"); enabled: !root.hub.busy && !Controller.mockMode; onClicked: root.hub.setAudioMute(device.kind,device.record.name,!device.record.muted) }
        }
        SettingsControls.Combo { property var ports: device.record.ports || []; visible: ports.length > 0; model: ports.map(function(port){ return port.description+(port.available ? "" : qsTr(" (bağlı değil)")) }); currentIndex: Math.max(0,ports.findIndex(function(port){return port.name === device.record.port})); enabled: !root.hub.busy && !Controller.mockMode; onActivated: if(ports[currentIndex].available) root.hub.setAudioPort(device.kind,device.record.name,ports[currentIndex].name) }
    }
    ColumnLayout {
        id: body; width: root.width; spacing: 14
        ColumnLayout {
            visible: root.page === "Sound"; Layout.fillWidth: true; spacing: 12
            SettingsControls.Title { text: qsTr("Çıkış · hoparlör ve kulaklık") }
            Repeater { model: root.state.outputs || []; delegate: AudioDevice { required property var modelData; record: modelData; kind: "sink" } }
            SettingsControls.Caption { visible: !(root.state.outputs || []).length; text: hub.refreshing ? qsTr("Ses aygıtları okunuyor…") : qsTr("Çıkış aygıtı bulunamadı. PipeWire/PulseAudio oturumunu kontrol et.") }
            SettingsControls.Title { text: qsTr("Giriş · mikrofon") }
            Repeater { model: root.state.inputs || []; delegate: AudioDevice { required property var modelData; record: modelData; kind: "source" } }
            SettingsControls.Caption { visible: !(root.state.inputs || []).length; text: qsTr("Kayıt için kullanılabilen mikrofon bulunamadı. Hoparlör izleme kaynakları mikrofon listesinde gösterilmez.") }
            SettingsControls.Title { text: qsTr("Uygulama sesleri") }
            Repeater {
                model: root.state.streams || []
                delegate: SettingsControls.Card { required property var modelData; SettingsControls.Label { text: modelData.name } SettingsControls.Slider { from: 0; to: 100; value: modelData.volume; enabled: !root.hub.busy && !Controller.mockMode; onMoved: if(!pressed) root.hub.setApplicationVolume(modelData.id,Math.round(value)); onPressedChanged: if(!pressed && !root.hub.busy) root.hub.setApplicationVolume(modelData.id,Math.round(value)) } }
            }
            SettingsControls.Caption { visible: !(root.state.streams || []).length; text: qsTr("Ses çalan uygulamalar burada görünür.") }
        }
        ColumnLayout {
            visible: root.page === "Power"; Layout.fillWidth: true; spacing: 12
            SettingsControls.Card {
                SettingsControls.Title { text: qsTr("Güç modu") }
                SettingsControls.Caption { text: qsTr("Yalnızca bu cihazın sunduğu güç profilleri uygulanabilir.") }
                RowLayout {
                    Layout.fillWidth: true
                    Repeater {
                        model: [{id:"power-saver",name:qsTr("Enerji tasarrufu"),icon:"battery-low-symbolic"},{id:"balanced",name:qsTr("Dengeli"),icon:"battery-good-symbolic"},{id:"performance",name:qsTr("Yüksek performans"),icon:"power-profile-performance-symbolic"}]
                        delegate: SettingsControls.Option { required property var modelData; text: modelData.name; iconName: modelData.icon; selected: root.state.profile === modelData.id; enabled: (root.state.profiles || []).includes(modelData.id) && !root.hub.busy && !Controller.mockMode; onClicked: root.hub.setPowerProfile(modelData.id) }
                    }
                }
                SettingsControls.Caption { visible: !(root.state.profiles || []).length; text: qsTr("Güç profili hizmeti veya donanım desteği bulunamadı. WSL/sanal makinelerde bu normal olabilir.") }
            }
            Repeater { model: root.state.batteries || []; delegate: SettingsControls.Card { required property var modelData; SettingsControls.Title { text: qsTr("Pil · %1%").arg(modelData.capacity) } SettingsControls.Caption { text: modelData.name+" · "+modelData.status } } }
            SettingsControls.Card { visible: !(root.state.batteries || []).length; SettingsControls.Caption { text: qsTr("Pil raporlayan bir aygıt yok; sistem masaüstü/sanal cihaz olarak çalışıyor.") } }
            OtherSettingsPage { category: "Power"; modernPower: true; Layout.fillWidth: true; Layout.preferredHeight: implicitHeight }
        }
        ColumnLayout {
            visible: root.page === "Keyboard & Mouse"; Layout.fillWidth: true; spacing: 12
            OtherSettingsPage { category: "Keyboard & Mouse"; Layout.fillWidth: true; Layout.preferredHeight: implicitHeight }
            SettingsControls.Card {
                SettingsControls.Title { text: qsTr("Fare ve dokunmatik yüzey") }
                SettingsControls.Toggle { label: qsTr("Birincil düğme sağda"); hint: qsTr("Sol el kullanımı"); checked: root.hub.input.leftHanded; enabled: !Controller.mockMode; onToggled: value => root.hub.setInput("leftHanded",value) }
                SettingsControls.Toggle { label: qsTr("Doğal kaydırma"); checked: root.hub.input.naturalScroll; enabled: !Controller.mockMode; onToggled: value => root.hub.setInput("naturalScroll",value) }
                SettingsControls.Toggle { label: qsTr("Dokunarak tıklama"); hint: qsTr("Dokunmatik yüzey destekliyorsa uygulanır"); checked: root.hub.input.tap; enabled: !Controller.mockMode; onToggled: value => root.hub.setInput("tap",value) }
                SettingsControls.Label { text: qsTr("İşaretçi hızı") }
                SettingsControls.Slider { from: -1; to: 1; stepSize: 0.05; value: root.hub.input.pointerSpeed; enabled: !Controller.mockMode; onMoved: if(!pressed) root.hub.setInput("pointerSpeed",value); onPressedChanged: if(!pressed) root.hub.setInput("pointerSpeed",value) }
            }
            SettingsControls.Card {
                SettingsControls.Title { text: qsTr("Tuş tekrarı") }
                SettingsControls.Label { text: qsTr("Tekrar hızı · %1 tuş/sn").arg(Math.round(rate.value)) }
                SettingsControls.Slider { id: rate; from: 1; to: 80; stepSize: 1; value: root.hub.input.repeatRate; enabled: !Controller.mockMode; onMoved: if(!pressed) root.hub.setInput("repeatRate",Math.round(value)); onPressedChanged: if(!pressed) root.hub.setInput("repeatRate",Math.round(value)) }
                SettingsControls.Label { text: qsTr("Tekrardan önce bekleme · %1 ms").arg(Math.round(delay.value)) }
                SettingsControls.Slider { id: delay; from: 200; to: 1500; stepSize: 50; value: root.hub.input.repeatDelay; enabled: !Controller.mockMode; onMoved: if(!pressed) root.hub.setInput("repeatDelay",Math.round(value)); onPressedChanged: if(!pressed) root.hub.setInput("repeatDelay",Math.round(value)) }
                SettingsControls.Caption { text: qsTr("Kişisel Labwc profiline kaydedilir. İç içe WSL oturumunda fare hızı gibi donanım davranışını Windows belirleyebilir.") }
            }
        }
        SettingsControls.Caption { visible: root.hub.error.length>0; text: root.hub.error; color: Controller.darkTheme ? "#FFB3A3" : "#A43138" }
    }
}
