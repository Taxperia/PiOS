# PiOS: Raspberry Pi 5 kurulum ve ilk masaüstü

PiOS, Raspberry Pi OS/Debian Trixie ARM64 tabanını kullanır. Normal bir yerel
kullanıcı hesabı oluşturulur; çevrimiçi PiOS hesabı gerekmez. Hazır kullanıcı
adı, fabrika parolası veya parolasız yönetici hesabı yoktur.

## İmajı yazma

Doğrulanmış `.img` dosyasını Raspberry Pi Imager'da **Use custom / Özel kullan**
ile seçip karta yazın. İmzalı sıkıştırılmış bir dosyayı açıyorsanız önce
sıkıştırılmış dosyanın imzasını ve sağlama toplamını doğrulayın.

Yerel bir özel imaj için Imager 2.x, işletim sistemi özelleştirmesini varsayılan
olarak kapatır. Bu projede hesap, ağ ve SSH ayarlarını PiOS sihirbazı yapar.
Imager'ın Raspberry Pi OS için sunduğu hesap/SSH özelleştirmelerinin PiOS'a
otomatik uygulanacağını varsaymayın. `userconf.txt`, `rpi-preseed.toml`,
`cloud-init` veya Imager'ın `firstrun.sh` formatı için bu sürümde bir PiOS
uyumluluk sözleşmesi yoktur.

[Imager'ın resmi özelleştirme formatları](https://github.com/raspberrypi/rpi-imager/blob/main/doc/os_customisation_formats.md).

## Monitör, klavye ve fareyle

1. Kartı Pi 5'e takıp açın. İlk cihaz kurulumunda dil, klavye, saat dilimi,
   hesap adı ve parola seçin. Ethernet kullanabilir, Wi-Fi seçebilir veya
   çevrimdışı devam edebilirsiniz.
2. İhtiyacınıza göre SSH ve tarayıcı erişimini açın. Bunlar başlangıçta kapalıdır.
   Güncellemeyi kontrol edin veya açıkça atlayın. Kurulumu tamamlayınca cihaz
   yeniden başlar.
3. Oluşturduğunuz kullanıcı adı ve parolayla giriş yapın. Masaüstü ilk
   açıldığında **PiOS'a hoş geldiniz** ekranı gelir. İleri ile dil/tarih/klavye,
   açık/koyu/otomatik tema, sekiz vurgu rengi, görev çubuğu, panel konumu ve
   tasarımı, animasyonlar, erişim tercihleri ve güncellemeler adımlarını gezin.
4. Tema ve panel ayarları hemen uygulanır. Oturum dili ve klavye değişiklikleri
   sonraki girişte uygulanır. Saat dilimi değişikliği sistem yetkilendirmesi
   isteyebilir. Tarih biçimi paneldeki tarih gösterimini değiştirir.
5. **Masaüstünü aç** tamamlandığını kullanıcı hesabına kaydeder; sonraki
   oturumlarda sihirbaz otomatik açılmaz. **Daha sonra devam et** ilerlemeyi
   korur. Ayarlar → Sistem → **PiOS karşılama ekranını aç** ile geri dönün.

Konum izni mevcut sağlayıcıya ve uygulamanın izin mekanizmasına bağlıdır.
Karşılama ekranı konum sorgulamaz; uygulama izinlerini incelemeye yönlendirir.
PiOS'ta henüz rapor gönderme hizmeti olmadığından tanılama paylaşımı kapalı ve
devre dışıdır; gelecekteki bir hizmet için izin kaydedilmez. Bu bölüm bir
telemetri veya tüm uygulamalara uygulanabilen genel konum anahtarı değildir.

## Monitör olmadan ilk kurulum

İmajı yazdıktan sonra FAT önyükleme bölümünü bilgisayara tekrar bağlayın.
Windows'ta Python 3 ile (örnek sürücü harfini kartın gerçek harfiyle değiştirin):

```powershell
python scripts/fidan-provision --boot E:\ --hostname fidan
```

Linux'ta:

```sh
python3 scripts/fidan-provision --boot /media/owner/bootfs --hostname fidan
```

Yardımcı tek kullanımlık kurulum parolası ister; karta açık parola yerine
tuzlanmış doğrulayıcı ve geçici Wi-Fi ağının PSK değerini yazar. Karta
yetkisiz erişimi önleyin. Bu işlem kartı biçimlendirmez veya imaj yazmaz.
Parola 12–63 UTF-8 bayt olmalıdır; ASCII harf/rakamlarla 12–63 karakter
seçebilirsiniz. **`Prepared https://fidan.local` mesajı görünmeden hazırlama
tamamlanmış sayılmaz.** Windows'ta `py` bulunamıyorsa yukarıdaki `python`
komutunu kullanın.

Pi'yi Ethernet'e bağlayıp `https://fidan.local` adresini açın. Ethernet yoksa
yardımcının yazdırdığı geçici Wi-Fi ağına bağlanıp `https://192.168.50.1`
adresini açın. Geçici AP desteği gerçek Pi donanımında ayrıca doğrulanmalıdır.
HTTPS sertifikası cihazda üretilir; ilk bağlantıda parmak izini güvenilen
bir yerel konsolla karşılaştırın. Ayrıntılar:
[tarayıcı ve başsız kurulum](headless-management.md).

11 Kasım 2026 tarihli yerel imajda Wi-Fi için `wpasupplicant` ve geçici AP
için `dnsmasq-base` eksiktir. Bu imajın ilk kurulumunu Ethernet ile yapın.
Hesabınızla erişim sağladıktan sonra `sudo apt update` ve
`sudo apt install wpasupplicant dnsmasq-base` komutlarıyla eksikleri kurun.
Sonraki imajlar için paket bağımlılıkları kaynakta düzeltildi; mevcut `.img`
dosyası bu değişiklikle yeniden üretilmedi.

## Kurulumdan sonra SSH ve tarayıcı

SSH'yi sihirbazdan veya Ayarlar → Gizlilik → SSH erişimi ile açın.
Gerekirse yerel terminalde `sudo fidan-ssh on` / `off` kullanın.
Windows PowerShell'den:

```powershell
ssh kullanici_adiniz@fidan.local
```

`.local` çözümlenemiyorsa aynı ağdaki Pi IP adresini kullanın. İlk SSH
bağlantısındaki sunucu anahtarını cihazdan doğrulayın. İmajda SSH host
anahtarları bulunmaz; her Pi kendine ait anahtarları üretir. Root ve boş
parolayla giriş kapalıdır. Kendi hesabınıza SSH açık anahtarı ekleyebilirsiniz.

Tarayıcı erişimini ayrıca açın: Ayarlar → Gizlilik → Tarayıcıdan masaüstü
erişimi veya `sudo fidan-remote configure on`. Ardından `https://fidan.local`
adresinde oluşturduğunuz yerel hesapla giriş yapın. Kurulum parolası kurulum
tamamlandığında kaldırılır. Bu, yerel ağda gerçek Wayland masaüstünü noVNC ile
gösterir; bir web uygulamasına dönüştürülmüş ayrı bir PiOS arayüzü değildir.

Windows Uzak Masaüstü için isteğe bağlı yol: [RDP kurulumu](rdp.md).

## GitHub güncellemeleri

`fidan-updater` zaten GitHub Releases üzerinden imzalı PiOS paketlerini altı
saatte bir denetler. Sabit güven anahtarı, imzalı manifest, SHA-256, Debian
paket kimliği/mimarisi ve sürüm ilerlemesi doğrulanır. Ayarlar → Sistem'den
otomatik güncellemeyi değiştirebilir, elle kontrol edebilir ve paket kurtarmayı
açabilirsiniz. Debian tabanı ve üçüncü taraf uygulamaların güncellemeleri
ayrıdır. Yeni kodun cihazlara ulaşması için bakımcının yeni paketleri üretip
güvenilen özel anahtarla imzalı GitHub sürümü yayımlaması gerekir; yalnızca
GitHub'a kaynak kodu göndermek bir güncelleme yayını oluşturmaz.

## Üretilen yerel imaj ve kalan donanım doğrulaması

14 Kasım 2026'da ARM64 çapraz derleme ve QEMU ile güncel imaj üretildi:

`D:\PiOS\pios-0.1.0-alpha.4-20261114\fidan-0.1.0-alpha.4-pi5-arm64.img`

Boyutu 4.194.304.000 bayttır (yaklaşık 3,91 GiB). Aynı klasördeki
`SHA256SUMS` ve `verification.json` bütünlük ve çevrimdışı kontrol sonuçlarını
içerir. Bölüm yapısı, iki gömülü dosya sisteminin birebir eşleşmesi, 51 varsayılan
ayar/firmware kontrolü ve kurulu ARM64 masaüstü, kurulum ve giriş uygulamalarının
QEMU açılışı doğrulandı. 23 yerel test grubu geçti. Tarayıcı gerçek masaüstünü ve
yerel kurulum penceresini gösterir. Çekirdek ve eşleşen initramfs imajın içindedir.
Bu yerel dosya bakımcı imzası taşımaz. SD karta yazma ve gerçek Pi 5 açılışı
henüz yapılmadı; Wi-Fi, HDMI, ses, gerçek hesap girişi ve uzaktan erişim donanımda
doğrulanmalıdır. İlk açılışta kök bölüm kartın kullanılabilir alanına genişletilir.

Yeniden üretmek için [AMD64 derleme rehberini](image-build-amd64.md), cihazda
doğrulamak için [donanım kabul listesini](hardware-test-checklist.md) izleyin.
