PiOS 0.1.0-alpha.6 improves the monitor-free browser desktop.

## Düzeltmeler

- Masaüstünün üstüne binen “Masaüstü kontrolleri” menüsü kaldırıldı. Bağlantı normalken tarayıcıdan ek bir panel görünmez; kurtarma seçenekleri yalnızca bağlantı kesilince gösterilir.
- Aktif kullanımdaki bağlantının yaklaşık 20 dakikada kesilmesi düzeltildi. Oturum, görünür ve yakın zamanda kullanılan masaüstünde yenilenir. Boşta kalma süresi, çıkış ve erişim iptali korunur; sekiz saatte bir yeniden giriş gerekir.
- Kısa ağ kesintilerinde beş denemeyle sınırlı, artan beklemeli otomatik yeniden bağlantı eklendi. Parola saklanmaz veya yeniden gönderilmez.
- Monitörsüz varsayılan ekran 1600×900, 30 Hz ve %125 ölçek olarak ayarlandı. Böylece uygulama pencereleri ve yazılar daha büyük görünür. Kaydedilmiş ekran tercihleri ve fiziksel monitör ayarları korunur.
- Görüntü aktarımı 25 FPS ile sınırlandı. Monitörsüz oturum artık her cihazda yazılımsal çizime zorlanmaz; erişilebilir bir DRM çizim aygıtı varsa kullanılır. GPU oturumu başlatılamazsa yazılımsal çizime dönülür.

## Güncelleme

Ayarlar → Sistem üzerinden güncellemeleri denetleyip indirerek kurun. Ardından **cihazı yeniden başlatın**; sanal ekran, çizim ve bağlantı servisleri yeni ayarları sonraki açılışta kullanır. Sonra tarayıcı sayfasını yenileyip yeniden giriş yapın. SD kartı yeniden yazmak gerekmez.

Bu sürüm Debian 13 için amd64/arm64 paketlerini ve imzalı güncelleme manifestini içerir; yeni bir IMG içermez. Pi üzerindeki gerçek gecikme ve GPU davranışı güncellemeden sonra cihazda ayrıca doğrulanmalıdır. Kare hızı sınırı ölçülmüş bir performans garantisi değildir.

Release trust fingerprint: `1085 5324 032F 285A FB73 CCD4 C41F F807 5817 D3B6`.
