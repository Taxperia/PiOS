# PiOS günlük uygulamaları

Uygulama menüsünde **PiOS PDF Görüntüleyici**, **PiOS Hesap Makinesi**, **PiOS Saat**
ve **PiOS Takvim** bulunur. Dördü de aynı PiOS tema/aksan tercihlerini izler ve
`fidan-apps` Debian paketiyle kurulur.

Yeni tam masaüstü kurulumunda meta-paket bu uygulamaları önerir. Mevcut PiOS
kurulumuna ilk kez eklerken `fidan-apps` paketini ayrıca kur; sonraki sürümlerini
PiOS güncelleyicisi kurulu diğer PiOS paketleri gibi günceller. Yeni paket zorunlu
bağımlılık yapılmadığından önceki güncelleyiciler mevcut masaüstünü güncelleyebilir.

```sh
fidan-apps --pdf '/home/fidan/Documents/belge.pdf'
fidan-apps --calculator
fidan-apps --clock
fidan-apps --calendar
```

- PDF: Yerel dosya açma/sürükleme, sayfa küçük resimleri, sayfa seçimi, metin
  arama (Ctrl+F), seçim kopyalama, yakınlaştırma, sığdırma, döndürme ve parola
  isteyen belgeler için parola penceresi. PDF düzenleme ve yazdırma yoktur.
  `application/pdf` için sistem varsayılanı eklenir; kullanıcının mevcut MIME
  tercihi korunur. Tercihi değiştirmek için
  `xdg-mime default org.fidan.Pdf.desktop application/pdf` kullanabilirsin.
- Hesap makinesi: Klavyeden veya düğmelerden işlem; işlem önceliği, parantez,
  karekök, üs, yüzde, bellek ve son 30 işlemin kalıcı geçmişi. `%` sayıyı 100'e
  böler: `50% = 0.5`, `200*10% = 20`. Finansal kesin ondalık aritmetik yerine
  çift duyarlıklı aritmetik kullanır. Virgül ve nokta ondalık ayırıcı olarak kabul edilir.
- Saat: Yerel saat, gerçek zaman dilimlerine göre altı dünya saati, tur kayıtlı
  kronometre ve duraklatılabilir sayaç. Sayaç sonunda masaüstü bildirimi gösterilir.
  Sayaç/kronometre için uygulama açık veya küçültülmüş kalmalıdır. Uyku sırasında
  bildirim gecikebilir; kapalı uygulamada arka plan alarm servisi çalışmaz.
- Takvim: Pazartesi başlayan aylık görünüm, bugün/ay gezinmesi, günlük/tüm gün
  etkinlikleri, düzenleme ve silme onayı. Veriler yereldir; hesap/bulut eşitlemesi
  ve etkinlik hatırlatıcısı yoktur. Etkinlikler
  `~/.local/share/fidan/calendar/events.json` içinde atomik olarak kaydedilir;
  eşzamanlı yazan pencereler dosya kilidi kullanır. Bozuk veri üzerine yazılmaz.

Geliştirme: `qt6-pdf-dev`, `qml6-module-qtquick-pdf` ve
`qt6-image-formats-plugin-pdf` gereklidir; `scripts/setup-debian-dev` bunları kurar.
Yeni test grubu `fidan-utilities-tests`, aritmetik hata durumlarını, takvim
kalıcılığını, sayaç/kronometreyi ve gerçek PDF render/aramasını doğrular.
