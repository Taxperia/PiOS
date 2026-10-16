# Ayarlar: alt sayfalar ve gerçek servisler

Sistem menüsünün tarih/saat, dil/bölge, kullanıcı, yedekleme ve gelişmiş ayarlar
bağlantıları artık Ayarlar içinde alt sayfa açar. Üstte geri düğmesi ve kategori
yolu bulunur; Alt+Sol da geri gider. Kapatma gibi onay isteyen işlemler ayrı
onay penceresini korur.

## Ekran

Fiziksel ekranlarda sürücünün ilan ettiği çözünürlükler gösterilir. Labwc'nin
`WL-` ve `HEADLESS-` sanal çıkışlarında 1024×768, 1280×720, 1366×768, 1600×900,
1920×1080, 2560×1440 ve 3840×2160 seçenekleri eklenir. Bunlar `wlr-randr`
özel modlarıdır: önce dry-run yapılır, desteklenmeyen istek hata döndürür.
WSLg iç içe ekranında yenileme hızını Windows/ana compositor belirler; bu yüzden
özel çözünürlüklerde sabit 60 Hz zorlanmaz, raporlanan otomatik hız kullanılır.
Görüntü değişince 15 saniyelik onay/geri alma ve yalnızca onaylanan profili
kaydetme korunur. Bu seçenekler fiziksel monitörün desteklemediği çözünürlüğü
destekliyormuş gibi ilan etmez.

## Ağ, ses ve güç

- Ağ ve internet: bağlantı özeti, Wi-Fi alt sayfası, Ethernet aygıtları, VPN ve
  gelişmiş ağ sayfaları. IPv4/MAC/MTU ve alınan/gönderilen byte değerleri gerçek
  aygıtlardan okunur. Sayaçlar arayüz açılışından beridir; aylık kota ölçümü değildir.
  Tünellerin trafiği fiziksel arayüz toplamına ikinci kez eklenmez. İnternet erişimi
  ancak NetworkManager `full` raporlarsa doğrulanmış gösterilir.
- VPN: kayıtlı NetworkManager VPN/WireGuard profillerini bağlama/kesme. Yeni
  profil, IP/DNS ve proxy düzenlemesi için kurulu bağlantı düzenleyicisi açılır.
  VPN sağlayıcı eklentileri ayrıca gerekebilir.
- Ses: PulseAudio uyumluluk arayüzü üzerinden PipeWire/PulseAudio aygıtları,
  varsayılan giriş/çıkış seçimi, ses/kayıt düzeyi, sessize alma, raporlanan portlar
  ve etkin uygulama sesleri. Hoparlör monitor kaynakları mikrofon diye listelenmez.
- Güç: `powerprofilesctl` tarafından gerçekten sunulan enerji tasarrufu, dengeli
  ve performans profilleri. Olmayan profiller seçilemez. Pil bilgisi yalnızca
  `/sys/class/power_supply` üzerinden pil raporlayan aygıtlarda görünür.

## Klavye ve fare

İşaretçi hızı, doğal kaydırma, sol el kullanımı, dokunarak tıklama, tuş tekrar
hızı ve gecikmesi `~/.config/fidan/input.conf` dosyasına kaydedilir. `fidan-input`
kişisel Labwc XML profilini atomik günceller ve yeniden yapılandırır; sistemin
salt okunur profilini değiştirmez. Yeni normal oturumlar kişisel profil kullanır.
İç içe WSL oturumunda Windows tarafından yönetilen fare davranışı değişmeyebilir.
Klavye düzeni mevcut PiOS düzen tercihini kullanır.

## Uygulamalar ve gizlilik

Yüklü uygulamalar, bu kullanıcıya görünen başlatılabilir desktop girişleri ve
Flatpak uygulamalarından oluşturulur. Kitaplık paketleri uygulama sayılmaz.
Sürüm ve paket boyutu raporlanabiliyorsa paket yöneticisinden okunur; yerel
kurulumlarda veri uydurulmaz. Ayrıntı ve varsayılan uygulamalar alt sayfaları vardır.

Gizlilikte Flatpak için ağ, geniş dosya paylaşımı, doğrudan tüm aygıtlar ve ses
sunucusu erişimi yönetilir. Dosya erişimini kapatma `host:reset` ile geniş
paylaşımları sıfırlar; belge portalıyla tek tek paylaşılan dosyalar ayrıdır.
Ses soketi hem çalma hem kaydı etkiler. Kamera/mikrofon, konum, bildirim ve arka
plan portalı kararları ayrıca düzenlenebilir. Bunlar ilgili portalı kullanan
uygulamalarda geçerlidir; bütün yerel Debian uygulamalarını sınırlayan bir
global izin duvarı değildir. Uygulamaları değişiklikten sonra yeniden aç.

Şifreleme durumu kök dosya sisteminin `lsblk` aygıt zincirinden okunur. LUKS,
şifrelenmemiş veya doğrulanamayan durum ayrılır. WSL'nin Windows BitLocker
durumu okunmuş sayılmaz. Çalışan disk bu ekrandan otomatik şifrelenmez; tam
disk şifrelemesi kurulumda/yedekleme ve kurtarma planıyla yapılandırılır.
Geçiş anahtarları için sağlayıcılar (tarayıcı, parola yöneticisi, FIDO anahtarı)
kullanılır; PiOS bunları ortak bir kasaya aktarmıyor.

Kaynaklar: [Flatpak izinleri](https://docs.flatpak.org/en/latest/sandbox-permissions.html),
[Portal izin deposu](https://github.com/flatpak/xdg-desktop-portal/wiki/The-Permission-Store),
[Labwc ayarları](https://labwc.github.io/labwc-config.5.html).

## Kendi varsayılan duvar kâğıtlarını ekle

1. Resimleri kaynakta `artwork/wallpapers/collection/` altına koy. PNG, JPG/JPEG,
   WebP ve SVG desteklenir; örneğin `benim-orman.png`. Dosya adından görünen ad üretilir.
2. Lisans/sahiplik kaydını `artwork/ASSET-LICENSES.md` içine ekle.
3. Kurulum bunları `/usr/share/backgrounds/fidan/collection/` altına kopyalar.
   Üretim sisteminde galerinin okuduğu dizin budur. Geliştirme derlemesi kaynak
   klasörünü de tarar. Ayarlar penceresini yeniden açarak listeyi yenileyebilirsin.
4. Yeni kurulumların varsayılanını değiştirmek için `config/product.json` içindeki
   `wallpaper` alanını `/usr/share/backgrounds/fidan/collection/benim-orman.png`
   yap. Mevcut kullanıcının kişisel duvar kâğıdı seçimi önceliklidir.

Mevcut ana resim `artwork/wallpapers/fidan-mountain-dawn-4k.png` dosyasıdır.
Galerine yeni resim eklemek için bu dosyanın üzerine yazman gerekmez.
