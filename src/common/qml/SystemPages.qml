// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root; property string category: "System"; readonly property string sheet: category.includes("/") ? category.split("/")[1] : ""
    signal navigate(string page)
    implicitHeight: sheet.length ? detailBody.implicitHeight : layout.implicitHeight
    onVisibleChanged: if (visible) SettingsData.refreshSystem()
    Component.onCompleted: SettingsData.refreshSystem()
    onCategoryChanged: if (visible) SettingsData.refreshSystem()
    Timer { interval: 30000; repeat: true; running: root.visible; onTriggered: SettingsData.refreshSystem() }
    function showSheet(name) { SettingsData.refreshSystem(); root.navigate("System/"+name) }
    ColumnLayout {
        id: layout; width: root.width; spacing: 14; visible: !root.sheet.length
        SettingsControls.Caption { text: root.category === "System" ? qsTr("Cihazının temel ayarlarını yönet.") : root.category === "Storage" ? qsTr("Disk kullanımını ve depolama ayarlarını yönet.") : qsTr("Cihazın ve işletim sistemin hakkında bilgiler.") }
        GridLayout {
            visible: root.category === "System"; Layout.fillWidth: true; columns: root.width>=760 ? 2 : 1; columnSpacing: 16; rowSpacing: 14
            ColumnLayout {
                Layout.fillWidth: true; Layout.preferredWidth: root.width*0.62; Layout.alignment: Qt.AlignTop; spacing: 9
                SettingsControls.Link { label: qsTr("Tarih ve saat"); hint: qsTr("Saat dilimi ve otomatik senkronizasyon"); value: SettingsData.info.time+"\n"+SettingsData.info.timezone; iconName:"preferences-system-time-symbolic"; onTriggered: root.showSheet("date") }
                SettingsControls.Link { label: qsTr("Dil ve bölge"); hint: qsTr("Sistem dili, bölge ve klavye düzeni"); value: SettingsData.info.locale; iconName:"preferences-desktop-locale-symbolic"; onTriggered: root.showSheet("region") }
                SettingsControls.Link { label: qsTr("Güç ve oturum"); hint: qsTr("Güç profili, kilitleme ve kapatma"); value: Controller.powerProfilesAvailable ? Controller.powerProfile : qsTr("Destek durumu"); iconName:"system-shutdown-symbolic"; onTriggered: root.navigate("Power") }
                SettingsControls.Link { label: qsTr("Kullanıcılar"); hint: qsTr("Hesap bilgileri ve oturum seçenekleri"); value: SettingsData.info.user; iconName:"avatar-default-symbolic"; onTriggered: root.showSheet("users") }
                SettingsControls.Link { label: qsTr("Yedekleme"); hint: qsTr("Kişisel dosyalarını koru ve geri yükle"); value: SettingsData.toolAvailable("backup") ? qsTr("Yönet") : qsTr("Kurulum gerekli"); iconName:"document-save-symbolic"; onTriggered: root.showSheet("backup") }
                SettingsControls.Link { label: qsTr("Gelişmiş ayarlar"); hint: qsTr("Depolama, aygıtlar ve gizlilik"); iconName:"preferences-system-symbolic"; onTriggered: root.showSheet("advanced") }
                SettingsControls.Card { SettingsControls.Caption { text: qsTr("ⓘ  Görünüm ve kişisel tercihler otomatik kaydedilir.") } }
            }
            ColumnLayout {
                Layout.fillWidth: true; Layout.preferredWidth: root.width*0.38; Layout.alignment: Qt.AlignTop; spacing: 12
                MachineCard {}
                SettingsControls.Card {
                    SettingsControls.Title { text: qsTr("Sistem güncellemeleri") }
                    SettingsControls.Caption { text: Updates.statusText }
                    SettingsControls.Toggle { label: qsTr("Otomatik güncellemeler"); hint: qsTr("GitHub'daki imzalı PiOS paketlerini 6 saatte bir kontrol et ve kur. Oturumun açık kalır."); checked: Updates.automaticEnabled; enabled: !Updates.busy && !Controller.mockMode; onToggled: value => Updates.setAutomaticEnabled(value) }
                    SettingsControls.Button { Layout.fillWidth: true; text: Updates.busy ? qsTr("İşlem sürüyor…") : Updates.downloaded ? qsTr("Güncellemeyi kur") : Updates.updateAvailable && Updates.installable ? qsTr("Güncellemeyi indir") : qsTr("Güncellemeleri kontrol et"); primary: true; enabled: !Updates.busy && !Controller.mockMode; onClicked: Updates.downloaded ? Updates.install() : Updates.updateAvailable && Updates.installable ? Updates.download() : Updates.check() }
                }
                SettingsControls.Link { label: qsTr("Daha fazla bilgi"); hint: qsTr("Sürüm, lisanslar ve yardım"); iconName:"help-about-symbolic"; onTriggered: root.navigate("About") }
            }
        }
        ColumnLayout {
            visible: root.category === "Storage"; Layout.fillWidth: true; spacing: 12
            SettingsControls.Card {
                RowLayout { Layout.fillWidth: true; spacing: 14; SettingsControls.Icon { name:"drive-harddisk-symbolic"; Layout.preferredWidth: 42; Layout.preferredHeight: 42 } ColumnLayout { Layout.fillWidth: true; SettingsControls.Title { text: qsTr("Sistem diski") } SettingsControls.Caption { text: SettingsData.volumes.length ? qsTr("Toplam %1 · Kullanılan %2 · Boş %3").arg(SettingsData.volumes[0].totalText).arg(SettingsData.volumes[0].usedText).arg(SettingsData.volumes[0].freeText) : qsTr("Depolama bilgisi alınamıyor") } Rectangle { Layout.fillWidth: true; implicitHeight: 10; radius: 5; color: Controller.darkTheme ? "#304B4B" : "#BCD3C5"; Rectangle { width: parent.width*(SettingsData.volumes.length ? SettingsData.volumes[0].percent/100 : 0); height: 10; radius: 5; color: Controller.accentColor } } } SettingsControls.Label { Layout.fillWidth: false; text: SettingsData.volumes.length ? Math.round(SettingsData.volumes[0].percent)+"%" : "—" } }
            }
            SettingsControls.Title { text: qsTr("Bölümler ve bağlı sürücüler") }
            SettingsControls.Card {
                Repeater { model: SettingsData.volumes; delegate: RowLayout { required property var modelData; Layout.fillWidth: true; spacing: 14; SettingsControls.Icon { name:"drive-harddisk-symbolic" } ColumnLayout { Layout.fillWidth:false; Layout.preferredWidth:root.width*0.24; Layout.maximumWidth:root.width*0.27; SettingsControls.Label { text: modelData.mount; font.bold: true } SettingsControls.Caption { text: modelData.filesystem+" · "+modelData.totalText } } Rectangle { Layout.fillWidth: true; Layout.preferredWidth:root.width*0.3; Layout.minimumWidth:40; implicitHeight: 7; radius: 4; color: Controller.darkTheme ? "#304B4B" : "#BCD3C5"; Rectangle { width: parent.width*modelData.percent/100; height: 7; radius: 4; color: modelData.percent>90 ? "#E8A45D" : Controller.accentColor } } SettingsControls.Caption { Layout.fillWidth: false; Layout.preferredWidth:root.width*0.20; Layout.maximumWidth:root.width*0.20; text: modelData.usedText+" / "+modelData.totalText; horizontalAlignment: Text.AlignRight } SettingsControls.Label { Layout.fillWidth: false; text: Math.round(modelData.percent)+"%" } } }
            }
            SettingsControls.Title { text: qsTr("Ek araçlar") }
            SettingsControls.Link { label: qsTr("Diskleri yönet"); hint: SettingsData.toolAvailable("storage") ? qsTr("Bölümler, sürücü bilgileri ve disk sağlığı") : qsTr("GNOME Disks veya bölüm yöneticisi kurulu değil"); iconName:"drive-harddisk-symbolic"; onTriggered: { if (SettingsData.toolAvailable("storage")) SettingsData.openTool("storage"); else root.showSheet("storageTools") } }
            SettingsControls.Link { label: qsTr("Depolama analizi"); hint: SettingsData.toolAvailable("usage") ? qsTr("Alan kaplayan klasörleri incele") : qsTr("Disk Usage Analyzer veya Filelight kurulu değil"); iconName:"view-statistics-symbolic"; onTriggered: { if (SettingsData.toolAvailable("usage")) SettingsData.openTool("usage"); else root.showSheet("storageTools") } }
            SettingsControls.Button { text: qsTr("Bilgileri yenile"); onClicked: SettingsData.refreshSystem() }
        }
        GridLayout {
            visible: root.category === "About"; Layout.fillWidth: true; columns: root.width>=760 ? 2 : 1; columnSpacing: 16; rowSpacing: 14
            MachineCard { Layout.preferredWidth: root.width*0.7; Layout.alignment: Qt.AlignTop }
            SettingsControls.Card {
                Layout.preferredWidth: root.width*0.3; Layout.alignment: Qt.AlignTop
                Image { Layout.alignment: Qt.AlignHCenter; Layout.preferredWidth: 64; Layout.preferredHeight: 64; source: "image://theme/org.fidan.Shell" }
                SettingsControls.Title { text: Product.name; horizontalAlignment: Text.AlignHCenter; font.pixelSize: 22 }
                SettingsControls.Caption { text: Product.version+" · "+Product.channel; horizontalAlignment: Text.AlignHCenter }
                SettingsControls.Caption { text: qsTr("Özgür yazılım, açık kaynak."); horizontalAlignment: Text.AlignHCenter }
                SettingsControls.Button { Layout.fillWidth: true; text: qsTr("Lisanslar"); onClicked: root.navigate("System/licenses") }
                SettingsControls.Button { Layout.fillWidth: true; text: qsTr("Sistem güncellemeleri"); onClicked: root.navigate("System") }
                SettingsControls.Button { Layout.fillWidth: true; text: qsTr("Yardım ve kısayollar"); onClicked: SettingsData.openHelp() }
                SettingsControls.Caption { text: qsTr("Debian ve açık kaynak topluluğuyla geliştirildi."); horizontalAlignment: Text.AlignHCenter }
            }
        }
        SettingsControls.Caption { visible: SettingsData.actionError.length>0; text: SettingsData.actionError; color: Controller.darkTheme ? "#FFB3A3" : "#A43138" }
    }
    SettingsControls.Card {
            id:detailBody; width:root.width; visible:root.sheet.length>0
            contentSpacing: 13
            ColumnLayout { visible: root.sheet === "date"; Layout.fillWidth: true; SettingsControls.InfoRow { label:qsTr("Yerel saat"); value: SettingsData.info.time } SettingsControls.InfoRow { label:qsTr("Saat dilimi"); value:SettingsData.info.timezone } SettingsControls.Toggle { label:qsTr("Otomatik saat eşitleme"); hint: SettingsData.info.ntpAvailable ? qsTr("Sistem yetkilendirmesi gerekebilir") : qsTr("Bu ortamda saat eşitleme hizmeti kullanılamıyor"); checked: SettingsData.info.ntpEnabled; enabled: SettingsData.info.ntpAvailable && !SettingsData.actionBusy; onToggled: value => SettingsData.setNtp(value) } }
            ColumnLayout { visible: root.sheet === "region"; Layout.fillWidth: true; SettingsControls.InfoRow { label:qsTr("Sistem dili"); value:SettingsData.info.locale } SettingsControls.Label { text:qsTr("Klavye düzeni") } SettingsControls.Combo { property var layouts:["us","tr","de","gb"]; model:["English (US)","Türkçe","Deutsch","English (UK)"]; currentIndex:Math.max(0,layouts.indexOf(Controller.keyboardLayout)); onActivated:Controller.keyboardLayout=layouts[currentIndex] } SettingsControls.Caption { text:qsTr("Klavye düzeni sonraki masaüstü oturumunda uygulanır. Sistem dili Debian dil paketleriyle yönetilir.") } }
            ColumnLayout { visible: root.sheet === "users"; Layout.fillWidth: true; SettingsControls.InfoRow { label:qsTr("Kullanıcı"); value:SettingsData.info.user } SettingsControls.InfoRow { label:qsTr("Ev klasörü"); value:SettingsData.info.home } SettingsControls.Caption { text: SettingsData.toolAvailable("users") ? qsTr("Hesap değişiklikleri yönetici yetkisi isteyebilir.") : qsTr("Bu sistemde grafik hesap yöneticisi kurulu değil.") } SettingsControls.Button { text:qsTr("Hesapları yönet"); enabled:SettingsData.toolAvailable("users"); onClicked:SettingsData.openTool("users") } }
            SettingsControls.Caption { visible: root.sheet === "backup" || root.sheet === "storageTools"; text: root.sheet === "backup" ? qsTr("Dosya yedeklerini yönetmek için Déjà Dup gibi bir yedekleme uygulaması kullanabilirsin. Burada otomatik yedekleme etkinleştirilmiş değildir.") : qsTr("Diskleri yönetmek için GNOME Disks, klasör boyutlarını incelemek için Disk Usage Analyzer kullanabilirsin. Bu araçlar ayrıca kurulur.") }
            SettingsControls.Button { visible:root.sheet === "backup" && SettingsData.toolAvailable("backup"); text:qsTr("Yedekleme uygulamasını aç"); onClicked:SettingsData.openTool("backup") }
            SettingsControls.Button { visible:root.sheet === "storageTools" || (root.sheet === "backup" && !SettingsData.toolAvailable("backup")); text:qsTr("Uygulama mağazasını aç"); onClicked:Controller.openSoftwareCenter() }
            ColumnLayout { visible:root.sheet === "advanced"; Layout.fillWidth:true; SettingsControls.Button { text:qsTr("Depolama"); onClicked:{ root.navigate("Storage") } } SettingsControls.Button { text:qsTr("Klavye ve fare"); onClicked:{ root.navigate("Keyboard & Mouse") } } SettingsControls.Button { text:qsTr("Gizlilik"); onClicked:{ root.navigate("Privacy") } } SettingsControls.Button { text:qsTr("Görev yöneticisi"); onClicked:Controller.openTaskManager() } }
            SettingsControls.Caption { visible:SettingsData.actionError.length>0; text:SettingsData.actionError; color:"#EAA495" }
            SettingsControls.Card { visible:root.sheet === "licenses"; SettingsControls.Title { text:"GNU GPL v3 · Project Fidan" } TextArea { Layout.fillWidth:true; implicitHeight:500; text:SettingsData.licenseText; readOnly:true; wrapMode:TextEdit.Wrap; selectByMouse:true; color:Controller.darkTheme ? "#F2F7F4" : "#18261F"; background:Item {} } }
        }
}
