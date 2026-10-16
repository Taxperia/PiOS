# PiOS güvenlik incelemesi — 16 Ekim 2026

PiOS kaynak kodu ve mevcut Debian/WSL kurulumu incelendi. Öncelik ilk kurulum servisi, root ile çalışan başlatıcı, güncelleyici, mağaza işlemleri, dosya açma ve kilit ekranıydı. Bulgular düzeltildi ve derlenen uygulamalar Debian’a kuruldu. Bu çalışma sırasında arayüz tasarımları değiştirilmedi.

## Düzeltilen bulgular

| Alan | Bulgu ve koşul | Uygulanan düzeltme |
| --- | --- | --- |
| İlk kurulum / hesap oluşturma | Kurulum hesabından gelen parola satır sonu içerebiliyordu. Root servisinin `chpasswd` girdisinde ikinci kullanıcı kaydı oluşabilirdi. | Kullanıcı adı tam eşleşmeyle doğrulanıyor; parola ve görünen adda CR, LF ve NUL reddediliyor. Geçerli Unicode adlar ve parola içindeki normal iki nokta korunuyor. |
| İlk kurulum / yetki süresi | Çalışmaya devam eden kurulum süreci tamamlanma işaretinden sonra da değişiklik isteyebiliyordu. | Tamamlanma işareti her mutasyon çağrısında kontrol ediliyor; kurulum hakları tamamlanınca sona eriyor. Yerel ayar ve saat dilimi girdileri de sıkılaştırıldı. |
| WSL başlatıcısı / root izinleri | Root, kullanıcının yazabildiği `/run/user/UID/fidan-x11` yoluna `chown` ve `chmod` uyguluyordu. Önceden yerleştirilen sembolik bağlantı başka bir dizinin izinlerini değiştirebilirdi. | Kaynak dizin root’un yönettiği `/run` altında `mktemp -d` ile özel ve tahmin edilemez oluşturuluyor. Kullanıcı bu dizinin üst dizinini değiştiremiyor. |
| Wi-Fi / parola gizliliği | Ayarlar parolayı `nmcli` komut satırı argümanı olarak iletiyordu; süreç incelemesinde görülebilirdi. | `--ask` ve özel standart giriş borusu kullanılıyor. Parola argüman listesine yazılmıyor; çok satırlı parola girdisi reddediliyor. |
| Masaüstü / dosya açma | İndirilen bir `.desktop` dosyası, çalıştırma izni olmadan simgeli bir belge gibi görünüp komut çalıştırabiliyordu. | Masaüstünde normal dosya, açık çalıştırma izni ve grup/diğer kullanıcıların yazamaması şartı aranıyor; sembolik bağlantı çalıştırılmıyor. Dosya yöneticisi güvenilmeyen kısayolları editörde açıyor; aynı izin kontrolünü geçen çalıştırılabilir kısayollar klasör içinden de açılabiliyor. Uygulama menüsünden oluşturulan mevcut güvenilir kısayollar çalıştırılabilir olarak üretilmeye devam ediyor. |
| Kilit ekranı / geçici dosya | Tahmin edilebilir CSS dosyası ve denetlenmeyen runtime yolu, önceden hazırlanmış sembolik bağlantılar üzerinden kullanıcı dosyasının üzerine yazılmasına yol açabilirdi. | Runtime dizininin sahibi ve 0700 izinleri doğrulanıyor, bağlantı izlenmiyor. Her çağrıya özel dizin ve 0600 CSS dosyası oluşturulup işlem sonunda temizleniyor. |

`chpasswd` girdisinin satır başına kullanıcı/parola kaydı olması [aracın kılavuzunda](https://www.man7.org/linux/man-pages/man8/chpasswd.8.html) tanımlanır. Geçici dosyalarda kullanılan özel oluşturma yaklaşımı [Python tempfile belgeleriyle](https://docs.python.org/3/library/tempfile.html) uyumludur.

## Ek korumalar

- Güncelleyici, root olarak kullanıcı girdisini kopyalarken sembolik bağlantıları, FIFO ve aygıtları reddediyor. Metadata 2 MiB, paket 2 GiB ile sınırlı; büyüyen dosyalarda da akış sınırı uygulanıyor.
- İndirme ve hazır işareti atomik, özel geçici dosyalarla yazılıyor. Başarısız indirme önceki dosyayı koruyor. Önbellekte farklı sahip veya grup/diğer kullanıcı yazma izni reddediliyor. İmza, SHA-256, paket kimliği ve geri sürüme dönmeme kontrolleri korundu.
- Mağazada APT kurulumuna `--no-remove` eklendi. APT’nin paket adına eklenen `-` işaretini kaldırma talebi olarak yorumlama davranışına ve bağımlılık çözümlemesinin mevcut paketleri kaldırmasına karşı koruma sağlar. [APT kılavuzu](https://manpages.debian.org/testing/apt/apt-get.8.en.html).

## Debian ve doğrulama

- İmzalı Debian güvenlik deposundan 28 paket güncellendi; hiçbir paket kaldırılmadı. Chromium, OpenSSL, curl, Ghostscript, Expat, Kerberos, librsvg, libjxl, xz ve NTFS bileşenleri dahildi.
- Son kontrol: kurulu paketler için bekleyen Debian güvenlik güncellemesi 0. APT’nin genel kararlı deposundaki diğer güncellemeler bu sayıya dahil değildir.
- Flatpak’ın güncelleme sorgusu bekleyen uygulama/güncelleme satırı döndürmedi.
- Kontrol anında TCP dinleme soketi görülmedi; SSH ve ilk kurulum servisi etkin değildi.
- Kaynaklarda özel anahtar ve yaygın erişim tokenı biçimleri dosya adı döndüren aramayla tarandı; eşleşme bulunmadı. Bu, tüm olası gizli bilgi biçimlerinin tarandığı anlamına gelmez.
- Tam derleme ve 10 CTest grubu başarılı. Güncelleyicinin 22, kilit ekranının 6 ve WSL başlatıcısının 1 Python testi buna dahil. C++ testleri parola enjeksiyonunu, güvenilmeyen kısayolları ve Wi-Fi parolasının argümanlarda bulunmamasını doğruluyor.
- Gerçek hesap parolaları testlerde değiştirilmedi; ayrı geçici test dosyaları ve sahte araçlar kullanıldı. Paket veritabanı denetimi temiz.

Yerel doğrulama kayıtları: `build/security-build.log`, `build/security-final-tests.log`, `build/security-updates-install.log`, `build/security-install.log`, `build/security-dpkg-audit.log`.
