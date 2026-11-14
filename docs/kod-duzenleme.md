# PiOS kodlarını kendin değiştirmek

Kaynak klasörün `C:\Users\talba\Desktop\debian`. Bu klasörü VS Code'da aç.
Dosyaları UTF-8 olarak kaydet. Windows'ta düzenleyebilir, Linux uygulamalarını
WSL Debian içinde derleyip deneyebilirsin. Raspberry Pi'ye her küçük değişiklikte
yeniden IMG yazman gerekmez.

## Hangi dosyayı değiştireceğim?

| Değişiklik | Dosya / klasör |
| --- | --- |
| Siyah Dark / Green ayrımı ve eski tema tercihlerinin taşınması | `src/common/Appearance.h` |
| Masaüstü renkleri, köşe yarıçapı, panel yüksekliği | `src/shell/qml/Theme.qml` |
| Bağımsız uygulamaların ortak renkleri | `src/common/AppStyle.h` |
| Tema, vurgu rengi, duvar kâğıdı seçenekleri | `src/common/qml/PersonalizationPage.qml` |
| Üst panel / sol dock | `src/shell/qml/components/TopBar.qml`, `src/shell/qml/surfaces/DockSurface.qml` |
| Kullanıcının masaüstü karşılama ekranı | `src/shell/qml/components/WelcomeWizard.qml` |
| Cihazın hesap oluşturma ve ilk kurulum ekranı | `src/shell/qml/components/DeviceWelcomeWizard.qml` |
| Hesap oluşturma ve sistem ayarlarının uygulanması | `src/firstboot/FirstBootService.cpp`, `scripts/fidan-setup-preferences` |
| Tarayıcı giriş sayfasının yazıları ve bağlantı davranışı | `data/remote/ui.js` |
| Tarayıcı sayfasının yerleşimi ve görünümü | `data/remote/index.html`, `style.css` |
| HTTPS, oturum ve deneme sınırı | `src/remote/gateway.py` |
| Linux hesabıyla giriş ve masaüstü yetkisi | `src/remote/broker.py` |
| Monitörsüz ekran boyutu / görüntü aktarımı | `scripts/fidan-remote-session` |
| Dosyalar / Fotoğraflar / Mağaza | `src/files/qml`, `src/photos/qml`, `src/store/qml` |
| Hesap makinesi, takvim, saat, PDF | `src/apps/qml` |
| Ürün sürümü | `config/product.json` |
| Paket sürümü | `packaging/debian/changelog`, `image/packages.list` |

`QML` dosyaları ekranı tarif eder; `C++` dosyaları uygulamanın davranışını,
`Python` ve kabuk betikleri sistem servislerini yönetir. Ekranda görünen bir yazı
için önce VS Code'da **Ctrl+Shift+F** ile o yazıyı ara. Çeviriyle kullanılan
`qsTr(...)` metinlerinde ilgili çeviri dosyalarını da güncelle.

## Küçük bir değişikliği denemek

Örneğin `Theme.qml` içindeki `radiusMedium: 12` değerini `16` yaparak ortak
köşe yuvarlaklığını değiştirebilirsin. Renkler için `Controller.themeColor(...)`
ve uygulamalarda `UiStyle.color(...)` çağrılarını koru; böylece Light, Dark ve
Green geçişi çalışır. QML dosyaları uygulamaya gömülür: kaydetmek tek başına
kurulu uygulamayı değiştirmez, yeniden derlemek gerekir.

WSL **Debian terminalinde**, proje klasöründe:

```sh
cd /mnt/c/Users/talba/Desktop/debian
cmake --preset dev
cmake --build --preset dev --parallel 2
./scripts/run-nested-session
```

Son komut ayrı bir deneme masaüstü açar. İlk hazırlık ve normal kullanıcı
gereksinimleri [geliştirme belgesinde](development.md) anlatılır. Deneme sırasında
kişisel dosyalarına gerçekten erişen uygulamalar açıldığını unutma; dosya silme
gibi işlemleri yalnızca deneme dosyalarında yap.

İlgili kontrolleri çalıştır:

```sh
ctest --test-dir build/dev --output-on-failure -R 'settings|desktop-ui|utilities'
```

Tarayıcı giriş görünümünü sistem hesabına dokunmadan Windows PowerShell'de dene:

```powershell
cd C:\Users\talba\Desktop\debian
python tests/serve_remote_ui.py
```

Tarayıcıda `http://localhost:8765/checks` aç. Bu yalnızca yerel bir test sunucusudur;
gerçek Pi bağlantısını ve Linux parola denetimini taklit eder. İşin bitince
terminalde **Ctrl+C** ile kapat. Gerçek sunucu testleri Linux'ta:

```sh
python3 -m unittest discover -s tests -p test_remote.py
```

## Değişikliği Raspberry Pi'ye geçirmek

`git push` yalnızca kaynak kodu GitHub'a gönderir. Pi'nin otomatik güncellemesi
**imzalı Release paketlerini** kurar. Bir sürüm hazırlarken ürün, Debian paket ve
imaj tarifindeki sürümleri eşleştir; [sürüm yayımlama adımlarını](release.md) uygula.
Release iş akışı amd64 ve arm64 paketlerini test eder ve güncelleme manifestini
imzalar. İmzalama anahtarını kaynak koda veya mesaja ekleme.

Pi'de **Ayarlar → Sistem → Güncellemeleri denetle** ile yayımlanan sürümü kur.
Servis, giriş ekranı veya görüntü aktarımı değiştiyse yeniden başlat ve tarayıcıyı
yenile. Sadece Windows'taki dosyayı değiştirmek Pi'deki kurulu kopyayı değiştirmez.

## Yanlış bir değişikliği bulmak

```sh
git status --short
git diff
```

Mevcut Windows klasöründe Git `dubious ownership` hatası verirse, bu proje
klasörüne özel güven kaydı PowerShell'de şöyle eklenir:

```powershell
git config --global --add safe.directory C:/Users/talba/Desktop/debian
```

Her seferinde küçük bir değişiklik yap ve önce VirtualBox/WSL'de dene. Çalışan
değişiklikleri açıklayıcı bir Git commit'iyle kaydet. Hata raporu için sınırlı log:

```sh
journalctl -b --no-pager -n 50 -u fidan-gateway -u fidan-remote-broker
```

Bu komut yeni satırları sürekli takip etmez. Parola ve özel anahtarları paylaşma.
