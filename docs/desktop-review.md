# Fidan masaüstü incelemesi — 15 Ekim 2026

Öncelik: görünüm ve kullanım kolaylığı. Otomatik kontrollerin ardından görünür
Debian masaüstünde fare ve klavye ile kullanım turu yapıldı. Aşağıdaki düzeltmeler
derlendi ve mevcut Debian ortamına kuruldu.
Bu kayıt fiziksel Raspberry Pi kabul testlerinin yerine geçmez.

## Disk erişimi yeniden sağlandı

C: üzerinde yer açıldıktan sonra Debian `WSL_E_DISK_CORRUPTED` hatasıyla
başlayamıyordu. Durdurulmuş sanal diskin tam kopyası alındı; dosya sistemi
bağlı değilken UUID ile doğrulandı ve `e2fsck -fp` ile günlük kurtarması yapıldı.
Ardından `e2fsck -fn` beş denetim geçişini hatasız, çıkış kodu 0 ile tamamladı.
Debian yeniden açıldı; paket araçları, derleme ve uygulamalar çalışıyor.

Onarım öncesi yedek korunuyor:
[`build/debian-before-repair-20261014.vhdx`](../build/debian-before-repair-20261014.vhdx)
(yaklaşık 8,4 GiB). Yedeğin SHA-256 değeri:
`E98924D776DB6A91A3B2E7ADB127E5DAF2A2B40D5C248FC02CD6E4BFCA376C53`.
Geçici kurtarma dağıtımı kaldırıldı. Debian C: üzerinde kaldı; D: sürücüsüne
taşınmadı. Son kontrolde C: üzerinde yaklaşık 20 GiB boş alan bulunuyordu.

WSL içindeki yaklaşık 1 TB sanal kapasite, Windows sürücüsündeki fiziksel boş
alanı göstermez. Kurtarma yaklaşımı için
[Microsoft WSL disk onarım belgesi](https://learn.microsoft.com/en-us/windows/wsl/disk-space#how-to-repair-a-vhd-mounting-error).

## Düzeltilen sorunlar

| Alan | Önceki davranış | Yeni davranış / doğrulama |
| --- | --- | --- |
| Masaüstü sıralaması | Sort By yalnızca bildirim gösteriyordu. | Ada, türe ve son değişiklik tarihine göre gerçek sıralama; tercih kaydedilir. Model testi geçti. |
| Klasör araması | Arama başka klasöre taşınıp dosyaları gizleyebiliyordu. | Başarılı klasör geçişinde arama temizlenir; sonuç bulunamadığında uygun metin gösterilir. Model ve QML testleri geçti. |
| Geri/ileri gezinme | Silinmiş hedefte geçmiş indeksi yanlış değişiyordu. | Geçmiş yalnızca hedef açıldığında ilerler. Silinen hedef ve yeniden deneme testi geçti. |
| Adres çubuğu | Geçersiz adres açık klasörden farklı görünüyordu. | Başarısız geçişte gerçek konuma döner. QML testi geçti. |
| Klavye kullanımı | Dosya oluşturma ve arama fare gerektiriyordu. | Ctrl+L, Ctrl+F, Ctrl+N, Ctrl+Shift+N, Alt+oklar ve F5; oluşturma/ad değiştirmede Enter. Gerçek QML penceresi testleri geçti. |
| Hata geri bildirimi | Oluşturma hatası iletişim kutusunun arkasında kalıyordu. | Oluşturma/ad değiştirme hataları açık iletişim kutusunda görünür. Geçersiz ad testi geçti. |
| Okunabilirlik | Dosya adları, arama ve adres yazıları küçüktü. | Bu alanlar 10 px'den 12 px'e çıkarıldı. Gerçek uygulama görüntüleri incelendi. |
| Açık tema | Görev düğmeleri ve hızlı ayarlarda düşük kontrast; Ayarlar başlığı koyu kalıyordu. | Görev düğmeleri, Ayarlar çerçevesi, hızlı ayar kutuları ve onay yüzeyi temaya uyar. Görüntüler kontrol edildi. |
| Ayarlar kontrolleri | Karanlık temada bazı girişler beyaz; anahtarlar gri görünüyordu. | Tema renklerini kullanan seçim/giriş alanları ve belirgin yeşil anahtarlar; desteklenmeyen parlaklık sürgüsü gizlenir. |
| Başlatıcı | Aramada Enter uygulamayı açmıyor; açma başarısız olsa da menü kapanıyordu. | Enter seçili sonucu açar; hata durumunda menü açık kalır ve bildirim gösterilir. Gerçek Wayland arama/Enter akışı geçti. |
| Mağaza kartları | Kesilen ad/açıklama okunamıyordu. | Üzerine gelindiğinde tam ad ve açıklama gösterilir. QML çalışma hatası taraması geçti. |
| Sistem göstergeleri | Ayrılmış bloklar kullanım sayılıyordu; sensör yokken 0°C görünüyordu. | Kullanım serbest bloklardan hesaplanır; sensör yoksa Unavailable gösterilir. Bağımsız dosya sistemi istatistiği testi geçti. |
| Wi-Fi durumu | Global radyo bayrağı Ethernet ortamında da Wi-Fi varmış gibi gösteriyordu. | Gerçek Wi-Fi aygıtı aranır. Bu WSL'de yalnızca Ethernet/loopback var; ağ sayfası No wireless adapter detected gösteriyor. |
| Hızlı ayar durumları | Açık radyo Connected; desteklenmeyen güç profili Balanced görünüyordu. | Radyo için On/Off; desteklenmeyen profil için devre dışı Unavailable gösterilir. |

## Tamamlanan kontroller

- `cmake --build --preset dev`: başarılı.
- `ctest --preset dev --output-on-failure`: 2/2 hedef geçti. Qt çıktısında
  son koşuda model testleri 26/26, dosya arayüzü testleri 10/10 (kurulum/kapanış dahil).
- Ayarlar'ın 11 sayfası açık/karanlık temada, masaüstü yüzde 100/125/150
  ölçekte; başlatıcı, hızlı ayarlar, Dosyalar, Mağaza ve Görev Yöneticisi'nin
  iki görünümü: 36 görüntü durumu üretildi; çalışma hatası taraması geçti.
- Gerçek Labwc/Wayland oturumu: dört Fidan uygulaması pencere açtı; Dosyalar
  küçültme/geri getirme/odaklama ve Alt+F4 kontrolleri geçti.
- Başlatıcıda test uygulamasını arama ve Enter ile açma geçti; açılan süreç
  normal `xdg-shell` pencere rolünü aldı.
- QTerminal gerçek `/dev/pts/*` terminali sağladı.
- Büyütme/geri alma Qt arayüz testinde geçti; compositor görüntüsü ayrıca
  kontrol edildi. Kurulu wlrctl'nin `contains_value()` fonksiyonu büyütülmüş
  durumun enum değeri 0'ı reddediyor; onun `state:maximized` sorgusu başarı
  ölçütü olarak kullanılmıyor.
- `flatpak repair --user --dry-run` tutarlılık kontrolü geçti; `dpkg --audit`
  sorun bildirmedi. Önceki turda APT ile Mousepad, Flatpak ile GIMP kuruldu ve
  gerçek uygulama pencereleri görüldü.
- Depo doğrulama betiği ve kaynak boşluk denetimi geçti.

Görüntüler: [`artifacts/desktop-review`](../artifacts/desktop-review).
Gerçek oturum sonucu: [`live/results.txt`](../artifacts/desktop-review/live/results.txt).
Örnek: [büyütülmüş Dosyalar](../artifacts/desktop-review/live/files-maximized.png),
[açık tema hızlı ayarlar](../artifacts/desktop-review/quick-settings-light.png).

Tekrar çalıştırmak için Debian terminalinde:

```sh
cmake --build --preset dev
ctest --preset dev --output-on-failure
./scripts/verify-desktop-ui
./scripts/test-nested-desktop
```

Görsel tarama kullanıcı tercihlerini ayrı geçici HOME içinde tutar. Gerçek
oturum testi ayrı test dosyaları ve test uygulaması kullanır; kullanıcı dosyaları
test verisi olarak kullanılmaz.

## Kalan kapsam

Windows bilgisayar kontrolü eklentisi `Computer Use native pipe is unavailable`
hatası veriyor. Etkileşimli tur Windows arayüzünü otomatikleştirmek yerine açık
Linux compositor'üne `wlrctl`/`wtype` ile gerçek fare ve klavye girdisi gönderdi;
her akışın ekranı `grim` ile alındı ve görsel olarak incelendi.

Fiziksel Pi açılışı, PAM kimlik doğrulaması, ilk kurulum, Wi-Fi/Bluetooth
eşleştirme, ses donanımı, gerçek güç işlemleri, uzun süreli performans,
imzalı sürüm yükseltme/geri alma ve başsız tarayıcı erişimi bu turda tamamlanmadı.
[Donanım kontrol listesi](hardware-test-checklist.md) ayrıca uygulanmalıdır.

## Görünür masaüstünde etkileşimli kullanım turu

Bu bölüm önceden üretilmiş arayüz resimlerinden ayrıdır: açık, kalıcı Labwc
oturumunda simgeler ve düğmeler tıklandı, arama ve metin alanlarına yazıldı,
dosyalar oluşturuldu ve işlemlerin sonucu ekrandan yeniden kontrol edildi.

| Akış | Sonuç / yeni düzeltme |
| --- | --- |
| Dosyalar → Documents | İlk tıklamada klasör bulunamadı. Bilinen yan menü konumları gerektiğinde oluşturulur; geçerli konum vurgulanır. Aynı tıklama tekrarlandı, klasör açıldı. Rastgele adresler otomatik oluşturulmaz. |
| Klasör/dosya oluşturma → F2 → sağ tık → Trash | Gerçek test dosyası oluşturuldu, yeniden adlandırıldı ve çöp kutusunda görüldü. Durum çubuğunun eski dosya adında kalması düzeltildi; yeni dosyada doğru ad ve boş klasörde boş durum doğrulandı. Oluşturma kutusundaki yinelenen beyaz başlık kaldırıldı. |
| Dosyayı editörde açma → yazma → Ctrl+S | FeatherPad açıldı. Test metni kaydedildi ve diskten aynı içerik okundu. Yalnızca test için oluşturulan iki dosya ve boş test klasörü kimlikleri doğrulanarak temizlendi. |
| Klasör → Open in Terminal | Gerçek QTerminal açıldı; `pwd` açılan klasörün yolunu gösterdi. |
| Dock → Web Browser | Kurulu olmayan uygulama sessizce hiçbir şey yapıyordu. Artık mağazadan kurulum açıklaması gösterilir. VLC için Flatpak masaüstü kimliği de aranır; gerçek VLC kurulumu bu turda denenmedi. |
| Başlatıcı → GIMP → Enter | Kısa adı uzun görünen adında geçmediği için GIMP bulunmuyordu. Aramaya masaüstü kimliği ve komut eklendi; `GIMP` sorgusunda sonuç görüldü. Gerçek Flatpak GIMP 3.2.6 ana penceresi açıldı. Sonuç bulunamama açıklaması da ekranda doğrulandı. |
| Mağaza | Featured, Installed, arama ve Updates sekmeleri tıklandı; kurulu GIMP/QTerminal/FeatherPad listelendi. Bu etkileşimli turda paket kaldırılmadı; önceki APT/Flatpak kurulum doğrulamaları yukarıda kayıtlıdır. |
| Ayarlar | 11 sayfa açıldı. Açık/koyu tema, masaüstü simgelerini gizleme/gösterme ve duvar kâğıdı seçicisini açıp iptal etme denendi. Yeniden başlatma onay kutusu açıldı ve iptal edildi. |
| Ayarlar → ölçek seçimi | Açık listede beyaz üstüne çok açık yazı görünüyordu. Seçim kutusu ve açılır liste tema renkleriyle çizilir. Koyu listede 100/125/150 seçenekleri tekrar açılarak okundu. |
| Hızlı ayarlar / başlatıcı / bildirim merkezi | Ölçek ayarı açılır yüzeylere de uygulanır, boyut ekran alanıyla sınırlanır. %150 hızlı ayarlar ekrana sığdı; başlatıcı ve geçmiş taşmadı. Önceki %125, koyu tema, görünür simgeler ve ilk çalışma alanı geri yüklendi. |
| Bildirim geçmişi | Koyu temada okunmayan Clear düğmesi düzeltildi. Test bildirimi görüldü, temizlendi ve boş geçmiş açıklaması doğrulandı. |
| Görev Yöneticisi | Süreç/PID arama ve canlı CPU/bellek grafikleri denendi. İki ayrı, bize ait `sleep` süreci arayüzden sonlandırıldı. Sonlandırılan PID'nin seçili kalması düzeltildi; yeniden denemede seçim temizlendi ve End task devre dışı kaldı. Onay başlığı da koyu temaya uyarlandı. |
| Pencereler / çalışma alanları | Başlık düğmesiyle büyütme ve küçültme, panelden geri getirme, Alt+F4 ve 1/2/3 çalışma alanlarına geçiş doğrulandı. Tarih sıralaması simgelerin sırasını değiştirdi; Name tercihi geri yüklendi. |

Örnek kanıtlar:
[Belgeler düzeltmesi](../artifacts/desktop-review/interactive-documents-fixed.png),
[GIMP kısa ad araması](../artifacts/desktop-review/interactive-launcher-gimp-fixed.png),
[gerçek GIMP](../artifacts/desktop-review/interactive-gimp-ready.png),
[okunabilir seçim listesi](../artifacts/desktop-review/interactive-choice-fixed.png),
[%150 hızlı ayarlar](../artifacts/desktop-review/interactive-quick-scaled-fixed.png),
[temizlenen görev seçimi](../artifacts/desktop-review/interactive-task-selection-cleared.png),
[bildirim düğmesi](../artifacts/desktop-review/interactive-notification-clear-fixed.png).

Bu ortamda `pactl info` bağlantıyı reddediyor; ses girişi/çıkışı doğrulanmadı.
Wi-Fi/Bluetooth aygıtı ve arka ışık denetimi yok. Bunlar başarılı donanım testi
olarak sayılmadı. İlk kurulum, oturum kilidinin kimlik doğrulaması ve gerçek
kapatma/yeniden başlatma uygulanmadı. Dosya kopyalama/yapıştırma ve çöp kutusundan
geri yükleme arayüzü henüz uygulanmamış özelliklerdir; çalıştıkları iddia edilmiyor.

Son derleme ve CTest 2/2 geçti; `git diff --check` temiz. Qt günlüklerinde
arama sırasında iki `DelegateModel::cancel: index out range` uyarısı ve katalog
testi kapanırken devam eden paket sorguları için iki QProcess uyarısı bulunuyor.
Bu koşuda çökme veya başarısız test olmadı; bu uyarılar giderilmiş sayılmadı.
