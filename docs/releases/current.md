PiOS 0.1.0-alpha.5 brings first setup into the real desktop and repairs monitor-free browser access. This is an engineering alpha.

## Yenilikler ve düzeltmeler

- **Tek ilk kurulum:** Dil, klavye, saat dilimi, hesap, ağ, tema, vurgu rengi, görev çubuğu, gizlilik ve güncelleme adımları normal duvar kâğıtlı masaüstünde tamamlanır. Seçimler yeni hesaba taşınır; aynı kurulum ikinci kez açılmaz. Eski mor kurulum, açıkça etkinleştirilen bir kurtarma seçeneği olarak korundu.
- **Monitörsüz kurulum:** Ethernet üzerinden tek kullanımlık kurulum parolasıyla gerçek masaüstüne erişim; cihaz başına hesap, TLS sertifikası ve SSH anahtarları. SSH ve kurulum sonrası tarayıcı erişimi kullanıcı tarafından seçilir. İsteğe bağlı RDP ayrıca açılabilir.
- **Tarayıcı masaüstü:** Siyah ekran/bağlantı kesilmesine yol açan yakalama başlatma hatası, servis kullanıcı kimliği ve fiziksel giriş aygıtı olmadan oturum başlatma düzeltildi. Kurulum hesabının eksik ev dizini onarılır. Yerel hesapla giriş, pano aktarımı ve bağlantı raporu desteği vardır.
- **Masaüstü:** Başlat menüsü, ses ve mikrofon denetimleri, bildirim göstergeleri, ekran görüntüsü araçları ve kişisel karşılama seçenekleri genişletildi. Dosya işlemleri ve uygulama ayarlarında düzeltmeler yapıldı.
- **Güncellemeler ve güvenlik:** GitHub sürümleri imzalı manifest ve paket özetleriyle doğrulanır. Güncelleme öncesinde kurulu PiOS paketlerinin kurtarma kopyası saklanır. Bağlantı raporları yalnızca yetkili kurulum oturumunda, sınırlı ve gizli alanlar temizlenerek alınabilir.

## Mevcut PiOS cihazını güncelleme

Ayarlar → Sistem bölümünde güncellemeleri denetleyip indirerek kurun. Terminalden aynı işlemler:

```sh
fidan-update check
fidan-update download
fidan-update install
```

Yeni masaüstü kodunun çalışması için işlemler başarıyla tamamlandıktan sonra oturumu yeniden açın veya cihazı yeniden başlatın. Güncelleyici açık masaüstünü zorla kapatmaz. Otomatik güncelleme zamanlayıcısı etkinse GitHub yaklaşık altı saatte bir denetlenir. İnternet bağlantısı gerekir.

Bu sürüm **Debian 13 için amd64 ve arm64 paketleri**, imzalı manifest ve sağlama toplamları içerir. **Yeni bir önyüklenebilir IMG içermez.** Mevcut kartı yeniden yazmak gerekmez. Fiziksel Raspberry Pi 5 kabul testlerinin tamamı henüz bitmemiştir.

Debian temel sistem paketleri ve üçüncü taraf uygulamalar kendi APT/Flatpak güncelleme yollarını kullanır. Mevcut hesaplar ve kişisel tercihler korunur. Önceden tamamlanmış bir kurulum yeniden başlatılmaz; kişisel karşılama henüz tamamlanmadıysa masaüstünde tamamlanabilir.

Release trust fingerprint: `1085 5324 032F 285A FB73 CCD4 C41F F807 5817 D3B6`.
