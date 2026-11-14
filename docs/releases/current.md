PiOS 0.1.0-alpha.7 adds separate Black Dark and Green appearances and improves sign-in feedback.

## Değişiklikler

- **Dark** siyah yüzeyler kullanır. Önceki yeşil görünüm **Green** adıyla korunur. Ayarlar → Kişiselleştirme bölümünden seçilir; ilk kurulumda da ikisi sunulur.
- Eski sürümdeki Dark tercihi Green'e taşınır; mevcut masaüstü görünümü ve vurgu rengi korunur. Açık uygulamalar yeni temayı yeniden açılmadan alır.
- Yerel hesap girişinde büyük/küçük harf ve baş/son boşluklar düzeltilir: `Taxperia` hesabı `taxperia` olarak denenir. Parola değiştirilmez.
- Çok fazla giriş denemesinde bekleme süresi ekranda geri sayılır. Düğme beklerken kilitlenir; parola kendiliğinden tekrar gönderilmez. Sunucudaki deneme sınırı ve hesap yetki kontrolleri korunur.
- Kaynak kodu düzenleme, dosyaların görevleri, test ve Pi'ye güncelleme adımları için Türkçe rehber eklendi: [kod düzenleme](../kod-duzenleme.md).

## Güncelleme

Pi'de **Ayarlar → Sistem** üzerinden denetleyip kurun, ardından yeniden başlatın ve tarayıcıyı yenileyin. SD kartı yeniden yazmak gerekmez. Şu an giriş sınırına takıldıysanız bir dakika bekleyip kullanıcı adını `taxperia` yazarak tekrar deneyin.

Bu Release, Debian 13 amd64/arm64 paketlerini ve imzalı güncelleme manifestini içerir. Raspberry Pi için yeni IMG içermez. VirtualBox amd64 OVA, Pi IMG'sinden ayrı bir dosyadır; [VirtualBox belgesine](../virtualbox.md) bakın.

Release trust fingerprint: `1085 5324 032F 285A FB73 CCD4 C41F F807 5817 D3B6`.
