# PiOS güvenlik incelemesi — 11 Kasım 2026

Kapsam: imaj sonlandırma, ilk kurulum hesabı ve D-Bus sınırı, yerel HTTPS
portalının Host/Origin/CSRF kontrolleri, PAM aracısı, imzalı güncelleme ve yeni
isteğe bağlı RDP yardımcıları. Bu bir kaynak ve izole test incelemesidir;
fiziksel Pi üzerinde sızma testi veya tüm bağımlılıkların CVE sertifikası değildir.

## Giderilenler

| Alan | Risk/eksiklik | Düzeltme |
| --- | --- | --- |
| İmaj kimliği | Paket kurulumu sırasında oluşturulmuş SSH host anahtarları, machine-id veya sistem random-seed imaja kalırsa kartlar aynı kimlikle çoğalabilir. | Sonlandırma anahtarları, D-Bus machine-id ve random-seed dosyasını kaldırır, `/etc/machine-id` dosyasını boşaltır. Cihazda eksik anahtarlar SSH başlamadan önce üretilir. |
| SSH erişimi | İmajın SSH sunucusu için proje tarafından açık bir root/boş parola politikası yoktu. | Root ve boş parola girişi kapatıldı; deneme sayısı ve giriş süresi sınırlandı. Yönetici doğrulaması gerektiren sabit `on/off` yardımcısı eklendi. |
| HTTPS Host ayrıştırması | Host URL ayrıştırıcısı sorgu/parça içeren bozuk değerleri de tanınan ana bilgisayar adı olarak kabul edebiliyordu. Bu bulgu tek başına tarayıcı tabanlı kimlik doğrulama atlatıldığını göstermez. | Sorgu/parça içeren Host değerleri reddedilir; negatif HTTPS testleri eklendi. |
| Tanılama onayı | Özel/eski D-Bus istemcisi mevcut olmayan tanılama servisi için onay kaydedebiliyordu. | `SetPrivacy` tanılama etkinleştirme taleplerini reddeder; yeni arayüz de mevcut olmayan servisi açıkça devre dışı gösterir. |
| İsteğe bağlı RDP | Standart xrdp paketi kurulurken tüm arayüzlerde dinlemeye başlayabilir ve ek oturumlar bellek tüketebilir. | Kurulum belgesi paketlerden önce servisleri maskeler. Yardımcı önce durdurur, loopback+TLS, hesap grubu, root yasağı, tek oturum ve zaman aşımları uygular; en son etkinleştirir. Varsayılan imaja RDP bağımlılıkları eklenmedi. |

## Korunan güven sınırları

- İlk kurulum tamamlanınca ayrıcalıklı kurulum çağrıları reddedilir; hesap ve
  Wi-Fi parolaları kurulum geçmişine yazılmaz, süreç argümanı olarak taşınmaz.
- HTTPS kurulum parolası kart başına seçilir. Tarayıcı masaüstü yerel hesap/PAM
  kullanır. Dış TCP VNC dinleyicisi yerine özel Unix soketi kullanılır.
- Güncelleyicinin güvenilen anahtar, imzalı manifest, SHA-256, paket kimliği,
  mimari, sürüm ilerlemesi ve kurtarma kontrolleri korundu. Güven denetimleri
  atlanmadı; uzaktaki rastgele betikleri çalıştıran bir güncelleme yolu eklenmedi.
- Yeni yardımcılar geliştirme bilgisayarında hesap, ağ, SSH veya RDP
  yapılandırması uygulamak için çalıştırılmadı. İzole testler geçici kökler ve
  sahte komut çalıştırıcıları kullanır.

## Kabul sınırları

11 Kasım doğrulaması: Debian 13 / AMD64 WSL kaynak derlemesi ve 21 CTest grubu
geçti (son tam çalışma: 106 saniye). Bunlara 26 güncelleyici, 15 uzaktan
erişim/RDP ve 6 imaj aracı kontrolü dahildir. Sonradan eklenen oturum dili ve
komut enjeksiyonu kontrolüyle 7 giriş-tercihi testi ayrıca geçti. Türkçe
karşılama ekranları 900×650 ve 600×500 boyutlarında, açık/koyu temada
görüntülendi. Shell modülü ekran yakalama çalışması uyarısız tamamlandı.
`all_qmllint` sıfır çıkış koduyla tamamlandı; mevcut bağlam/tip uyarıları
devam ediyor. Windows'ta geçici klasöre kart hazırlama doğrulandı. CMake
kurulum çıktısında yeni dosyalar ve Debian paket eşlemeleri kontrol edildi;
bu ilk kaynak doğrulamasından sonra 13 ARM64/all paket ve Pi 5 `.img` dosyası
da üretildi. Gerçek dosya sistemindeki 35 çevrimdışı denetim; fabrika hesabı
olmamasını, root kilidini, boş machine-id/SSH kimliklerini, ilk kurulumun
etkinliğini, SSH ve tarayıcı erişiminin varsayılan olarak kapalılığını ve Pi 5
önyükleme dosyalarını doğruladı. Dosya sistemi kontrolleri ve gömülü bölüm
özetleri geçti. Yerel imaj imzalı bir sürüm olarak yayımlanmadı.

Gerçek Pi 5'te kart başına farklı SSH/machine-id, SSH/PAM girişi, tarayıcı
görüntüsü, TLS kimliği, Wi-Fi AP, RDP/Xorg ve RAM ölçümleri hâlâ yapılmalıdır.
Debian/Raspberry Pi OS bağımlılıkları üretim sırasında depolardan alındı; paket
manifesti ve SBOM kaydedildi. Bu, bağımlılıkların kapsamlı bir CVE denetiminden
geçtiği anlamına gelmez; yayımlamadan önce ayrıca denetlenmelidir.
Karşılama ekranında genel bir konum anahtarı veya tanılama gönderim servisi
uygulanmış gibi gösterilmez. Ayrıntılar: [kurulum](pi5-kurulum.md),
[RDP](rdp.md), [imaj araçları](../image/README.md).
