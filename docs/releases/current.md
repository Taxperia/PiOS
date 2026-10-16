PiOS 0.1.0-alpha.3 improves everyday desktop use and includes security fixes. This is an engineering alpha.

## Yenilikler ve düzeltmeler

- **Masaüstü:** Boş alanda sol tuşla dikdörtgen çizerek toplu seçim, Ctrl+tıklama ve Ctrl+A. Seçili dosyaları ve uygulama kısayollarını klasörlere taşıma; Ctrl ile sürükleyerek kopyalama. Masaüstünün tablet gibi kayması giderildi. Klasöre taşınan güvenilir uygulama kısayolları açılabilir.
- **Mağaza:** 40 uygulamalık katalog, uygulama detayları, kurulum/kaldırma/güncelleme aşamaları, gerçek işlem yüzdesi ve hata ayrıntıları. APT ve Flatpak üzerinden gerçek kurulumlar yapılır. Mevcut yeşil mağaza görünümü korunmuştur.
- **Ayarlar:** Klavye dili ve diğer seçim menülerindeki çökme düzeltildi.
- **Görev yöneticisi:** CPU ve bellek grafikleri, aranabilir ve sıralanabilir süreç listesi, süreç ayrıntıları, duraklatma ve onaylı sonlandırma.
- **Güvenlik:** İlk kurulumda girdi ve yetki denetimleri; Wi-Fi parolalarının işlem argümanlarından çıkarılması; güvenilir kısayol denetimi; kilit ekranı, WSL başlatıcısı ve güncelleyicide geçici dosya/sembolik bağlantı korumaları. APT kurulumları mevcut paketleri kaldıramaz. Ayrıntılar: [güvenlik incelemesi](https://github.com/Taxperia/PiOS/blob/v0.1.0-alpha.3/docs/security-review-2026-10-16.md).

## Packages and updates

The assets include Debian 13 packages for **amd64** and **arm64**, a signed package manifest, SHA-256 checksums and the public release key. This release does **not** contain a bootable Raspberry Pi image; physical Pi 5 acceptance testing is still pending.

Install the packages for your architecture on a Debian 13 system. For an existing development desktop, install the artwork, shell, store, photos, task-manager, session, lock and updater packages. The `fidan-desktop` meta-package also installs the login/first-boot components and is intended for a dedicated desktop system.

Existing PiOS installations with the updater enabled check GitHub every six hours and update installed PiOS packages after signature verification. Installation never forces a reboot or restarts the active desktop. Debian base-system packages and third-party APT/Flatpak applications retain their own update mechanisms; local Debian security updates described in the review are not bundled in these assets.

Release trust fingerprint: `1085 5324 032F 285A FB73 CCD4 C41F F807 5817 D3B6`.
