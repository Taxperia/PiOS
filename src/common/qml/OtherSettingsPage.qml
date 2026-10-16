// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root; property string category: "Bluetooth"; property string feedback:""
    implicitHeight: layout.implicitHeight
    ColumnLayout {
        id: layout; width: root.width; spacing: 14
        SettingsControls.Caption { text: ({Bluetooth:qsTr("Kablosuz aygıtlarını bağla ve yönet."),Sound:qsTr("Ses düzeyini ve giriş/çıkış aygıtlarını ayarla."),Power:qsTr("Enerji kullanımını ve masaüstü oturumunu yönet."),"Keyboard & Mouse":qsTr("Yazma ve işaretleme deneyimini düzenle."),Applications:qsTr("Uygulamaları ve dosya açma tercihlerini yönet."),Privacy:qsTr("Bildirimlerini ve oturum güvenliğini kontrol et.")})[root.category] || "" }
        SettingsControls.Card {
            visible:root.category === "Bluetooth"
            RowLayout { Layout.fillWidth:true; SettingsControls.Icon { name:"bluetooth-symbolic"; Layout.preferredWidth:42; Layout.preferredHeight:42 } SettingsControls.Toggle { label:qsTr("Bluetooth"); hint:Controller.bluetoothAvailable?qsTr("Yakındaki aygıtlar için Bluetooth'u aç"):qsTr("Bu ortamda Bluetooth bağdaştırıcısı yok"); checked:Controller.bluetoothEnabled && Controller.bluetoothAvailable; enabled:Controller.bluetoothAvailable; onToggled:Controller.toggleBluetooth() } }
        }
        SettingsControls.Link { visible:root.category === "Bluetooth"; label:qsTr("Aygıtları eşleştir ve yönet"); hint:qsTr("Kulaklık, klavye, fare ve diğer aygıtlar"); iconName:"bluetooth-symbolic"; enabled:Controller.bluetoothAvailable; onTriggered:Controller.openSystemTool("bluetooth") }
        SettingsControls.Card {
            visible:root.category === "Sound"
            RowLayout { Layout.fillWidth:true; SettingsControls.Icon { name:Controller.volumeMuted?"audio-volume-muted-symbolic":"audio-volume-high-symbolic"; Layout.preferredWidth:42; Layout.preferredHeight:42 } ColumnLayout { Layout.fillWidth:true; SettingsControls.Title { text:qsTr("Çıkış sesi") } SettingsControls.Caption { text:Controller.audioAvailable?qsTr("Varsayılan hoparlör veya kulaklık"):qsTr("Kullanılabilir ses sunucusu bulunamadı") } } SettingsControls.Label { Layout.fillWidth:false; text:Controller.volume+"%"; font.pixelSize:24 } }
            SettingsControls.Slider { from:0; to:100; value:Controller.volume; enabled:Controller.audioAvailable; onMoved:Controller.volume=Math.round(value) }
            SettingsControls.Button { text:Controller.volumeMuted?qsTr("Sesi aç"):qsTr("Sessize al"); enabled:Controller.audioAvailable; onClicked:Controller.toggleVolumeMute() }
        }
        SettingsControls.Link { visible:root.category === "Sound"; label:qsTr("Giriş, çıkış ve uygulama sesleri"); hint:qsTr("Hoparlör, mikrofon ve uygulama başına ses düzeyi"); iconName:"audio-input-microphone-symbolic"; onTriggered:Controller.openSystemTool("sound") }
        SettingsControls.Card {
            visible:root.category === "Power"
            SettingsControls.Title { text:qsTr("Güç profili") }
            RowLayout { Layout.fillWidth:true; SettingsControls.Icon { name:"battery-good-symbolic"; Layout.preferredWidth:36; Layout.preferredHeight:36 } ColumnLayout { Layout.fillWidth:true; SettingsControls.Label { text:Controller.powerProfilesAvailable?Controller.powerProfile:qsTr("Bu cihazda güç profilleri desteklenmiyor") } SettingsControls.Caption { text:qsTr("Donanımın desteklediği enerji profili kullanılır") } } SettingsControls.Button { text:qsTr("Profili değiştir"); enabled:Controller.powerProfilesAvailable; onClicked:Controller.cyclePowerProfile() } }
        }
        SettingsControls.Card {
            visible:root.category === "Power"
            SettingsControls.Title { text:qsTr("Oturum") }
            SettingsControls.Caption { text:qsTr("Yeniden başlatmadan veya kapatmadan önce çalışmalarını kaydet.") }
            RowLayout { Layout.fillWidth:true; SettingsControls.Button { text:qsTr("Ekranı kilitle"); enabled:Controller.lockAvailable; onClicked:Controller.lockSession() } SettingsControls.Button { text:qsTr("Yeniden başlat…"); enabled:Controller.powerActionsAvailable && !Controller.mockMode; onClicked:{ power.action="restart"; power.open() } } SettingsControls.Button { text:qsTr("Kapat…"); enabled:Controller.powerActionsAvailable && !Controller.mockMode; onClicked:{ power.action="shutdown"; power.open() } } }
        }
        SettingsControls.Card {
            visible:root.category === "Keyboard & Mouse"
            SettingsControls.Title { text:qsTr("Klavye düzeni") }
            SettingsControls.Combo { objectName:"keyboard-layout"; readonly property var layouts:["us","tr","de","gb"]; model:["English (US)","Türkçe","Deutsch","English (UK)"]; currentIndex:Math.max(0,layouts.indexOf(Controller.keyboardLayout)); onActivated:Controller.keyboardLayout=layouts[currentIndex] }
            SettingsControls.Caption { text:qsTr("Seçilen düzen sonraki masaüstü oturumunda uygulanır.") }
            TextField { id:testInput; Layout.fillWidth:true; implicitHeight:42; placeholderText:qsTr("Klavyeni burada dene…"); selectByMouse:true; color:Controller.darkTheme?"#F2F7F4":"#18261F"; background:Rectangle { radius:8; color:Controller.darkTheme?"#102825":"#F4FAF6"; border.width:1; border.color:testInput.activeFocus?Controller.accentColor:"#426053" } }
        }
        SettingsControls.Link { visible:root.category === "Keyboard & Mouse"; label:qsTr("Klavye ve fare aygıtları"); hint:qsTr("İşaretçi, tuş tekrarı ve aygıt seçenekleri"); iconName:"input-mouse-symbolic"; enabled:SettingsData.toolAvailable("input"); onTriggered:Controller.openSystemTool("input") }
        SettingsControls.Card { visible:root.category === "Keyboard & Mouse"; SettingsControls.Title { text:qsTr("Kısayollar") } Repeater { model:[{name:qsTr("Uygulamalar arasında geçiş"),key:"Alt + Tab"},{name:qsTr("Pencereyi kapat"),key:"Alt + F4"},{name:qsTr("Dosyalar"),key:"Super + E"},{name:qsTr("Ayarlar"),key:"Super + I"},{name:qsTr("Ekranı kilitle"),key:"Super + L"},{name:qsTr("Görev yöneticisi"),key:"Ctrl + Shift + Esc"}]; SettingsControls.InfoRow { required property var modelData; label:modelData.name; value:modelData.key } } }
        SettingsControls.Link { visible:root.category === "Applications"; label:qsTr("Uygulama mağazası"); hint:qsTr("Debian ve Flatpak uygulamalarını kur, aç veya kaldır"); iconName:"system-software-install-symbolic"; onTriggered:Controller.openSoftwareCenter() }
        SettingsControls.Card {
            visible:root.category === "Applications"
            SettingsControls.Title { text:qsTr("Varsayılan uygulamalar") }
            SettingsControls.Caption { text:qsTr("Dosya türlerini hangi uygulamanın açacağını seç") }
            GridLayout { Layout.fillWidth:true; columns:root.width>760?2:1; columnSpacing:20; rowSpacing:14
                Repeater { model:root.category === "Applications" ? [{label:qsTr("Web tarayıcısı"),mime:"x-scheme-handler/http"},{label:qsTr("Metin dosyaları"),mime:"text/plain"},{label:qsTr("PNG fotoğrafları"),mime:"image/png"},{label:qsTr("MP4 videoları"),mime:"video/mp4"},{label:qsTr("MP3 ses dosyaları"),mime:"audio/mpeg"}] : []
                    ColumnLayout { id:app; required property var modelData; readonly property var choices:Controller.applicationChoices(modelData.mime); property string currentDefault:Controller.defaultApplication(modelData.mime); Layout.fillWidth:true; SettingsControls.Label { text:app.modelData.label } SettingsControls.Combo { model:app.choices.map(function(choice){return choice.name}); currentIndex:app.choices.findIndex(function(choice){return choice.id===app.currentDefault}); displayText:currentIndex>=0?currentText:app.currentDefault||qsTr("Uygulama seç"); enabled:app.choices.length>0; onActivated:{ const selected=app.choices[currentIndex].id; if(Controller.setDefaultApplication(app.modelData.mime,selected)){ app.currentDefault=selected; root.feedback=qsTr("Varsayılan uygulama kaydedildi.") } } } }
                }
            }
        }
        SettingsControls.Card { visible:root.category === "Privacy"; SettingsControls.Title { text:qsTr("Bildirimler") } SettingsControls.Toggle { label:qsTr("Bildirim başlıklarını göster"); hint:qsTr("Bildirim geçmişine üst panelden ulaşabilirsin"); checked:Controller.notificationsEnabled; onToggled:value=>Controller.notificationsEnabled=value } }
        SettingsControls.Card { visible:root.category === "Privacy"; SettingsControls.Title { text:qsTr("Oturum güvenliği") } SettingsControls.Caption { text:qsTr("Gerçek ekran kilidi, Debian hesabının parolası ve Wayland oturum kilitleme protokolünü kullanır.") } SettingsControls.Button { text:qsTr("Şimdi kilitle"); enabled:Controller.lockAvailable; onClicked:Controller.lockSession() } }
        SettingsControls.Caption { visible:root.feedback.length>0; text:root.feedback; color:Controller.accentColor }
    }
    Dialog { id:power; property string action:""; parent:Overlay.overlay; anchors.centerIn:parent; modal:true; title:action==="restart"?qsTr("Yeniden başlatılsın mı?"):qsTr("Bilgisayar kapatılsın mı?"); standardButtons:Dialog.Ok | Dialog.Cancel; SettingsControls.Label { width:320; text:qsTr("Devam etmeden önce açık çalışmalarını kaydet.") } onAccepted:Controller.requestPowerAction(action) }
}
