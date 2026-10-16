# PiOS güvenlik incelemesi — 17 Ekim 2026

Kaynak kodu ve mevcut Debian 13 / WSL kurulumu yeniden incelendi. Öncelik yeni
PDF görüntüleyici, giriş ve görüntü yardımcıları, uygulama izinleri, mağaza,
güncelleyici, ilk kurulum yetkileri ve dinleyen servislerdi. Mevcut arayüz
tasarımı korunarak aşağıdaki düzeltmeler Debian'a kuruldu.

## Düzeltilenler

| Alan | Koşul ve risk | Düzeltme |
| --- | --- | --- |
| PDF dış bağlantıları | Kullanıcı belgedeki bağlantıyı tıkladığında Qt'nin varsayılan bileşeni `file:` ve özel uygulama protokollerini de sistem URL işleyicisine gönderiyordu. Sonuç, kurulu işleyiciye göre yerel dosya veya uygulama açılması olabilir; bu, yalnızca PDF açılmasıyla otomatik kod çalıştırma bulgusu değildir. | Dış bağlantılar PiOS denetiminden geçer. Yalnızca geçerli, sunucusu bulunan HTTP/HTTPS adresleri açılır; yerel dosyalar, özel protokoller, göreli adresler ve kullanıcı bilgisi içeren yanıltıcı URL'ler engellenir. Belge içi sayfa bağlantıları korunur. |
| Gece ışığı dosyaları | Paylaşılan/yanlış izinli runtime dizinlerinde önceden hazırlanmış kilit, günlük veya tahmin edilebilir geçici dosya bağlantıları başka dosyaları ezme riski taşıyordu. Normal 0700 kullanıcı runtime dizini bu saldırının koşulunu zaten daraltıyordu. | Runtime sahibi ve 0700 izinleri doğrulanır; sembolik bağlantılar izlenmez. Kilit/günlük dosyaları sahibi doğrulanmış, tek bağlantılı normal dosyalar olmalıdır. Eski kullanıcı kilitleri içerikleri silinmeden 0600'e geçirilir. JSON kayıtları sınırlı okunur ve rastgele adlı özel dosyalarla atomik yazılır. |
| Gece ışığı süreçleri | Bozulmuş/sahte bir kayıt, aynı kullanıcıya ait ilgisiz bir süreci gamma süreci gibi tanıtabiliyordu. | Durdurma için UID, PID, gerçek komut satırı ve beklenen `wlsunset -o EKRAN` başlangıcı birlikte doğrulanır. Yardımcı root olarak çalışmayı reddeder. Bu bulgu, normal kullanıcıya yeni root yetkisi vermiyordu. |
| Güncelleyici dayanıklılığı | Paket ve imza dosyaları sınırlıyken GitHub sürüm listesinin JSON yanıtı sınırsız okunuyordu. | API yanıtı akış sırasında 2 MiB ile sınırlandırıldı. Bu, imza doğrulamasını değiştirmeyen kaynak tüketimi korumasıdır. |

Qt PDF 6.8.2'nin `PdfMultiPageView.qml` bileşeni, lisans/copyright başlıkları
korunarak `src/apps/qml/SafePdfMultiPageView.qml` içine alındı. Dış bağlantı açma
satırı bir PiOS sinyaline yönlendirildi; geri kalan görüntüleme davranışı
korundu. Bu kopya Qt güncellemelerinde ayrıca gözden geçirilmelidir. Gerçek PDF
ayrıştırıcısı sistemdeki Qt PDF kitaplığı olmaya devam eder.

[Qt PDF bileşeninin özelleştirme belgesi](https://doc.qt.io/qt-6/qml-qtquick-pdf-pdfmultipageview.html)
ve [Qt URL işleyicilerinin davranışı](https://doc.qt.io/qt-6/qdesktopservices.html).

## Debian yamaları ve ağ

İmzalı Debian paket indeksleri yenilendi. Paket kaldırmadan dört kurulu paket
güncellendi:

- `gzip`: `1.13-1+deb13u1` — CVE-2026-41991 / CVE-2026-41992.
- `libsqlite3-0`: `3.46.1-7+deb13u2` — FTS5 için CVE-2026-11822 / CVE-2026-11824.
- `dhcpcd-base`: `1:10.1.0-11+deb13u4` — CVE-2026-14258 dahil Debian düzeltmeleri.
- `linux-libc-dev`: `6.12.113-1` — güvenlik deposundaki başlık paketi. Bu,
  Windows tarafından sağlanan çalışan WSL çekirdeğinin güncellendiği anlamına gelmez.

Kaynaklar: [gzip](https://security-tracker.debian.org/tracker/source-package/gzip),
[SQLite](https://security-tracker.debian.org/tracker/source-package/sqlite3),
[dhcpcd](https://security-tracker.debian.org/tracker/source-package/dhcpcd).

Avahi'nin `enable-wide-area=yes` ayarı `no` yapıldı. Bu,
[CVE-2024-52615](https://security-tracker.debian.org/tracker/CVE-2024-52615) ve
[CVE-2024-52616](https://security-tracker.debian.org/tracker/CVE-2024-52616) için
Debian'ın belirttiği risk azaltma yöntemidir; Avahi ikilisinin kendisine yama
uygulandığı iddia edilmiyor. Aynı varsayılan yeni imajların hazırlama kancasına
eklendi. Geniş alan DNS-SD keşfi kapalıdır; yerel mDNS yapılandırması korunur.
Servis yeniden başlatıldı ve çalışır durumu doğrulandı. Önceki ayar root'a özel
`/etc/avahi/avahi-daemon.conf.pios-before-20261017` dosyasında saklandı.

Kontrol sırasında TCP dinleme soketi görülmedi. SSH sunucusu kurulu/etkin
değildi; ilk kurulum root servisi devre dışı ve çalışmıyordu. `fidan` için
parolasız `NOPASSWD` sudo kuralı yoktu. Kontrol edilen root yardımcılarında
grup/diğer kullanıcı yazma izni görülmedi. Flatpak'ın kullanıcı güncelleme
sorgusu boş döndü; paket veritabanı denetimi temizdi.

## Kalan takip kayıtları ve sınırlar

1568 kurulu Debian ikili paketi, 940 kaynak paket üzerinden
[Debian'ın resmi takip verisi](https://security-tracker.debian.org/tracker/data/json)
ile karşılaştırıldı. Otomatik eşleştirmede, yalnız Linux başlıklarına ait kayıtlar
ve `unimportant` kayıtları dışarıda bırakılınca 719 aday kayıt kaldı: 708 `open`,
11 `undetermined`. Bunlar **719 doğrulanmış veya kullanılabilir PiOS açığı**
anlamına gelmez. Kaynak paket, kurulu olmayan sunucu bileşenlerini veya devre
dışı özellikleri de kapsayabilir. Örneğin OpenSSH kayıtlarının bir kısmı sshd'ye
aitken bu kurulumda yalnız istemci vardır. Listedeki her bileşen için ayrı bir
erişilebilirlik/istismar analizi yapılmadı.

Bu adaylar içinde trixie için henüz sabit sürüm bildirilmiş ve kurulu sürümün
geride kaldığı ek bir kayıt bulunmadı. Buna karşılık Chromium, Qt, SQLite ve başka
bağımlılıklarda henüz trixie yaması bulunmayan veya değerlendirmesi süren kayıtlar
vardır; bunlar kapatılmış sayılmadı. 717 adayın önem derecesi henüz atanmamıştı;
bu durum düşük risk güvencesi vermez. Avahi'nin yukarıdaki iki kaydı, yapılandırma
ile azaltılmış olsa da sürüm tabanlı listede görünmeye devam eder.

Ham eşleştirme ve koşullar `artifacts/security-20261017/debian-tracker-findings.json`
dosyasındadır. Windows/WSL çekirdeği, GPU sürücüsü, Flatpak uygulamalarının iç
bağımlılıkları ve Git geçmişi bu sürüm karşılaştırmasıyla taranmış sayılmaz.
Kaynak ağacında yaygın özel anahtar/token biçimleri dosya adı döndüren aramayla
kontrol edildi; eşleşme bulunmadı. Derleme ve Git dizinleri bu aramada dışarıda
bırakıldı. Bu çalışma tam kapsamlı bir sızma testi veya zararlı yazılım taraması değildir.

## Doğrulama

- Tam derleme ve 15 CTest grubunun tamamı başarılı.
- Gerçek bir PDF bağlantı açıklaması okunup tıklama sinyali işlendi; yerel
  `.desktop` URL'sinin sistem işleyicisine gitmediği doğrulandı. Test, dosya veya
  uygulama çalıştırmadan sahte URL işleyicisi kullanır.
- 9 gece ışığı güvenlik testi; 24 güncelleyici testi başarılı. Gerçek hesap
  parolaları, Flatpak izinleri ve ekran gamma ayarı testlerde değiştirilmedi.
- QML denetim hedefi başarılı; mevcut bağlam erişimi uyarıları var.
- Kurulan uygulamanın derleme kimliği ve yardımcıların içerikleri kaynak
  derlemesiyle karşılaştırıldı. Gece ışığının mevcut kullanıcıda durum sorgusu başarılı.

Kayıtlar: `build/security-20261017-build.log`,
`build/security-20261017-final-tests.log`, `build/security-20261017-qmllint.log`,
`build/security-20261017-updates-install.log`,
`build/security-20261017-tracker-summary.json`.
