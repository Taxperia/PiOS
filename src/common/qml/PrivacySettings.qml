// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
Item {
    id: root
    property string page: "Privacy"
    readonly property var hub: SettingsData.desktop
    readonly property var flatpaks: (hub.state.applications || []).filter(function(app){return (app.flatpak || "").length>0})
    readonly property string selectedApp: appSelector.currentIndex>=0 && appSelector.currentIndex<flatpaks.length ? flatpaks[appSelector.currentIndex].flatpak : ""
    readonly property var permissions: { const snapshot=hub.state; return hub.permissions(root.selectedApp) }
    signal navigate(string page)
    implicitHeight: body.implicitHeight
    Component.onCompleted: hub.refresh()
    ColumnLayout {
        id: body; width: root.width; spacing: 12
        ColumnLayout {
            visible: root.page === "Privacy"; Layout.fillWidth: true; spacing: 12
            SettingsControls.Card { SettingsControls.Title { text: qsTr("Güvenlik ve kişisel veriler") } SettingsControls.Caption { text: qsTr("Gerçek sistem durumu ve uygulanabilen izinler burada gösterilir. Debian uygulamalarının tümüne uygulanabilen ortak bir sensör/hesap izni anahtarı yoktur.") } }
            SettingsControls.Link { label: qsTr("Cihaz şifrelemesi"); hint: qsTr("Sistem diskinin LUKS durumunu ve dosya koruma seçeneklerini incele"); iconName: "drive-harddisk-symbolic"; onTriggered: root.navigate("Privacy/Encryption") }
            SettingsControls.Link { objectName: "privacyPermissions"; label: qsTr("Uygulama izinleri ve kamera"); hint: qsTr("Flatpak ağ, dosya, aygıt, ses ve kamera portalı erişimleri"); iconName: "camera-web-symbolic"; onTriggered: root.navigate("Privacy/Permissions") }
            SettingsControls.Link { label: qsTr("Mikrofon"); hint: qsTr("Giriş aygıtlarını ve gerçek mikrofon sessize alma durumunu yönet"); iconName: "audio-input-microphone-symbolic"; onTriggered: root.navigate("Sound") }
            SettingsControls.Link { label: qsTr("Konum, hesap ve diğer veriler"); hint: qsTr("Erişim kapsamları, uygulama ve tarayıcı izinleri"); iconName: "mark-location-symbolic"; onTriggered: root.navigate("Privacy/Data") }
            SettingsControls.Link { label: qsTr("Parolalar ve geçiş anahtarları"); hint: qsTr("Parola kasası ve FIDO/WebAuthn desteği"); iconName: "dialog-password-symbolic"; onTriggered: root.navigate("Privacy/Credentials") }
            SettingsControls.Card { SettingsControls.Toggle { label: qsTr("Bildirim başlıklarını göster"); checked: Controller.notificationsEnabled; onToggled: value => Controller.notificationsEnabled=value } SettingsControls.Button { text: qsTr("Ekranı kilitle"); enabled: Controller.lockAvailable; onClicked: Controller.lockSession() } }
        }
        ColumnLayout {
            visible: root.page === "Privacy/Encryption"; Layout.fillWidth: true; spacing: 12
            SettingsControls.Card {
                SettingsControls.Title { text: hub.state.encryption === "encrypted" ? qsTr("Sistem diski LUKS zincirinde") : hub.state.encryption === "unencrypted" ? qsTr("Sistem diski LUKS ile şifrelenmemiş") : qsTr("Şifreleme durumu doğrulanamadı") }
                SettingsControls.Caption { text: qsTr("Durum bağlı kök dosya sisteminin blok aygıt zincirinden okunur; tüm ek diskler için bir garanti değildir. WSL sanal diskinin Windows BitLocker durumu buradan okunamaz.") }
                SettingsControls.Caption { text: qsTr("Çalışan sistemi tek düğmeyle şifrelemek veri taşıma ve kurtarma anahtarı yönetimi gerektirir. Tam disk koruması için Debian kurulumunda şifreli LVM/LUKS seç. Yedeklerini ve kurtarma bilgilerini ayrı tut.") }
                SettingsControls.Button { text: qsTr("Diskleri incele"); enabled: SettingsData.toolAvailable("storage"); onClicked: SettingsData.openTool("storage") }
            }
            SettingsControls.Card { SettingsControls.Title { text: qsTr("Dosyaları koruma") } SettingsControls.Caption { text: qsTr("Hesap parolası ve dosya izinleri oturum erişimini korur; disk şifrelemesi ise kapalı cihazdaki veriyi korur. Belgeler için ayrıca şifreli bir kasa uygulaması kullanılabilir.") } }
        }
        ColumnLayout {
            visible: root.page === "Privacy/Permissions"; Layout.fillWidth: true; spacing: 12
            SettingsControls.Caption { text: qsTr("Bu kontroller Flatpak uygulamalarına uygulanır. Değişiklikten sonra uygulamayı yeniden aç. Kamera portalı kararı, doğrudan aygıt erişimini tek başına engellemez.") }
            SettingsControls.Combo { id: appSelector; objectName: "privacyAppSelector"; model: root.flatpaks.map(function(app){return app.name}); enabled: root.flatpaks.length>0 && !hub.busy }
            SettingsControls.Card {
                visible: root.selectedApp.length>0
                SettingsControls.Title { text: root.selectedApp }
                enabled: Boolean(root.permissions.available)
                SettingsControls.Caption { visible: !root.permissions.available; text: qsTr("Bu uygulamanın izinleri okunamadı. Durum doğrulanmadan izin değiştirilemez; sayfayı yeniden açıp tekrar dene.") }
                SettingsControls.Toggle { label: qsTr("Ağ erişimi"); checked: Boolean(root.permissions.network); enabled: !hub.busy && !Controller.mockMode; onToggled: value => hub.setFlatpakPermission(root.selectedApp,"network",value) }
                SettingsControls.Toggle { label: qsTr("Geniş dosya erişimi"); hint: qsTr("Açma ev klasörünü paylaşır. Kapama geniş dosya izinlerini sıfırlar; dosya seçiciyle verilen erişimler ayrıdır."); checked: Boolean(root.permissions.files); enabled: !hub.busy && !Controller.mockMode; onToggled: value => hub.setFlatpakPermission(root.selectedApp,"files",value) }
                SettingsControls.Caption { visible: (root.permissions.filesystemSummary || "").length>0; text: root.permissions.filesystemSummary || "" }
                SettingsControls.Toggle { label: qsTr("Tüm aygıtlara doğrudan erişim"); hint: qsTr("Kamera/USB gibi aygıtların geniş erişim izni. Portal üzerinden verilen izinler ayrı yönetilir."); checked: Boolean(root.permissions.devices); enabled: !hub.busy && !Controller.mockMode; onToggled: value => hub.setFlatpakPermission(root.selectedApp,"devices",value) }
                SettingsControls.Toggle { label: qsTr("Ses sunucusuna erişim"); hint: qsTr("PulseAudio/PipeWire uyumluluk soketi; hem ses çalma hem mikrofon kaydını etkiler. Ayrı bir mikrofon anahtarı değildir."); checked: Boolean(root.permissions.audio); enabled: !hub.busy && !Controller.mockMode; onToggled: value => hub.setFlatpakPermission(root.selectedApp,"audio",value) }
                SettingsControls.Label { text: qsTr("Kamera portalı izni") }
                SettingsControls.Combo { readonly property var decisions: ["ask","yes","no"]; model: [qsTr("Yeniden sor / kayıt yok"),qsTr("İzin ver"),qsTr("Engelle")]; currentIndex: Math.max(0,decisions.indexOf(root.permissions.camera || "ask")); enabled: !hub.busy && !Controller.mockMode; onActivated: hub.setCameraPermission(root.selectedApp,decisions[currentIndex]) }
                SettingsControls.Label { text: qsTr("Diğer portal izinleri") }
                SettingsControls.Caption { text: qsTr("Yalnızca ilgili portalı kullanan uygulama ve hizmetlerde geçerlidir. Kayıt yokluğu tüm kaynaklar için engelleme anlamına gelmez. Konum izni IP adresinin paylaşılmasını engellemez.") }
                Repeater {
                    model: [{id:"microphone",name:qsTr("Mikrofon cihaz portalı")},{id:"location",name:qsTr("Konum portalı")},{id:"notification",name:qsTr("Bildirim portalı")},{id:"background",name:qsTr("Arka planda çalışma portalı")}]
                    delegate: ColumnLayout {
                        id: permissionRow; required property var modelData; Layout.fillWidth: true
                        SettingsControls.Label { text: permissionRow.modelData.name }
                        SettingsControls.Combo {
                            readonly property var decisions: ["ask","yes","no"]
                            readonly property string stored: root.permissions[permissionRow.modelData.id] || "ask"
                            model: [qsTr("Varsayılan / kayıt yok"),qsTr("İzin ver"),qsTr("Engelle")]
                            currentIndex: permissionRow.modelData.id === "location" ? stored === "NONE" ? 2 : stored === "ask" ? 0 : 1 : Math.max(0,decisions.indexOf(stored))
                            enabled: !hub.busy && !Controller.mockMode
                            onActivated: hub.setPortalPermission(root.selectedApp,permissionRow.modelData.id,decisions[currentIndex])
                        }
                    }
                }
            }
            SettingsControls.Card { visible: !root.flatpaks.length; SettingsControls.Caption { text: hub.refreshing ? qsTr("Uygulamalar okunuyor…") : qsTr("Yönetilebilen Flatpak uygulaması bulunamadı. Yerel Debian uygulamaları bu izin modeliyle sınırlandırılmaz.") } }
        }
        ColumnLayout {
            visible: root.page === "Privacy/Data"; Layout.fillWidth: true; spacing: 12
            SettingsControls.Card { SettingsControls.Title { text: qsTr("Konum") } SettingsControls.Caption { text: qsTr("Konum izni uygulama/portal veya tarayıcı tarafından istenir. Ağ/IP adresinden çıkarılan konum, GPS erişiminden ayrıdır. Bu masaüstünde bütün yerel uygulamalara uygulanabilen bir konum kapatma hizmeti yoktur.") } }
            SettingsControls.Card { SettingsControls.Title { text: qsTr("Hesap bilgileri, kişiler, dosyalar ve geçmiş") } SettingsControls.Caption { text: qsTr("Debian uygulamaları aynı kullanıcı hesabının erişebildiği verileri okuyabilir. Flatpak uygulamalarında dosya erişimini yukarıdaki izinlerden sınırlayabilirsin. Tarayıcı kamera, mikrofon, konum, bildirim ve site verisi izinleri tarayıcının site ayarlarında yönetilir.") } }
            SettingsControls.Card { SettingsControls.Title { text: qsTr("PiOS verileri") } SettingsControls.Caption { text: qsTr("Takvim ve hesap makinesi verileri kullanıcı hesabında yerel tutulur. Diğer uygulama ve servislerin veri politikaları ayrıca geçerlidir.") } }
            SettingsControls.Button { text: qsTr("Flatpak izinlerine git"); onClicked: root.navigate("Privacy/Permissions") }
        }
        ColumnLayout {
            visible: root.page === "Privacy/Credentials"; Layout.fillWidth: true; spacing: 12
            SettingsControls.Card { SettingsControls.Title { text: qsTr("Parola kasası") } SettingsControls.Caption { text: qsTr("Kurulu KeePassXC veya Seahorse ile parolalarını yönet. Kasaların içeriği bu ayarlar uygulamasına okunmaz.") } SettingsControls.Button { text: qsTr("Parola yöneticisini aç"); enabled: SettingsData.toolAvailable("credentials"); onClicked: SettingsData.openTool("credentials") } }
            SettingsControls.Card { SettingsControls.Title { text: qsTr("Geçiş anahtarları · FIDO/WebAuthn") } SettingsControls.Caption { text: qsTr("Geçiş anahtarları destekleyen tarayıcı, parola yöneticisi veya güvenlik anahtarıyla yönetilir. PiOS şu anda bütün sağlayıcıları kapsayan yerleşik bir geçiş anahtarı kasası sunmuyor.") } }
        }
        SettingsControls.Caption { visible: hub.error.length>0; text: hub.error; color: Controller.darkTheme ? "#FFB3A3" : "#A43138" }
    }
}
