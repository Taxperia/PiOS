# VS Code açılışı ve normal kullanıcı geçişi

15 Ekim 2026. Mağaza kurulumu tamamlanmıştı: `com.visualstudio.code` 1.139.1.
Açılış çıktısı şu hatayı verdi:

```text
Running as root without --no-sandbox is not supported.
```

Kullanıcının açık onayıyla `fidan` (UID 1000) hesabı oluşturuldu ve varsayılan
WSL kullanıcısı yapıldı. Hesap sudo grubundadır; parola atanmadı. Yönetici
kimlik doğrulaması gereken işlemlerden önce kullanıcı Windows terminalinden
şu komutla kendi parolasını etkileşimli olarak belirlemelidir:

```powershell
wsl -d Debian -u root -- passwd fidan
```

Mevcut 6 GB Flatpak deposu aynı dosya sisteminde
`/root/.local/share/flatpak` konumundan
`/home/fidan/.local/share/flatpak` konumuna taşındı ve yeni hesaba ait yapıldı.
Uygulamalar yeniden indirilmedi ve depo ikinci kez kopyalanmadı. VS Code,
GIMP 3.2.6 ve VLC 3.0.23 yeni hesapta listelendi. Kişisel root belgeleri yerinde
korundu; masaüstü/belge klasörleri, küçük uygulama profilleri ve mevcut masaüstü
tercihleri yeni hesaba kopyalandı. Root hesabı silinmedi.

`scripts/run-nested-session` artık kullanıcı tercihlerini `~/.config` altında
tutar. WSL'de eski bir root terminalinden çağrıldığında mevcut normal hesaba
geçer. Normal hesap bulunmayan root başlangıcı açıklayıcı hatayla durur.
`scripts/setup-debian-dev` de Flatpak uygulamalarını yanlış hesaba kurmamak için
normal kullanıcı gerektirir. `/etc/wsl.conf` önceki içeriği
`build/wsl-before-normal-user.conf` dosyasında saklıdır.

Doğrulama:

- Derleme ve CTest 2/2 geçti: model kontrolleri 27/27, dosya arayüzü 10/10
  (kurulum/kapanış dahil). Önceki turda kullanım sınırında kalan mağaza düğmesi
  ve canlı çıktı/başarısız indirme kontrolleri de bu derlemeye dahildir.
- `flatpak repair --user --dry-run` taşınan depoyu hatasız doğruladı.
- Başlatıcıdan Visual Studio Code arandı, Enter ile açıldı; uygulama süreçleri
  `fidan` hesabına ait. İlk açılış tamamlandı, hesap bağlantısı yapılmadan
  düzenleyiciye geçildi ve gerçek proje README dosyası açıldı.
- WSL varsayılan kullanıcı değişikliği ardından yeni `wsl -d Debian -- id`
  komutu UID 1000 gösterdi; HOME `/home/fidan` oldu. Kalıcı ayarla yeniden
  başlatılan masaüstünde VS Code bir kez daha açıldı.
- `git diff --check` temiz. Root ve normal kullanıcıya ait uygulama depolarının
  ikisi birden oluşturulmadı.

Kanıt: [çalışan VS Code düzenleyicisi](../artifacts/desktop-review/vscode-readme-working.png),
[başlatıcı](../artifacts/desktop-review/vscode-launcher-normal-user.png).
