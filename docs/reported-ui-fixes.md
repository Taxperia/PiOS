# Bildirilen arayüz sorunları — 15 Ekim 2026

Kullanıcının bildirdiği beş konuya yönelik çalışma kaydı.

| Konu | Uygulanan değişiklik ve doğrulama |
| --- | --- |
| Ayarlar yan menüsünde ikon yok | 11 kategoriye tema ikonları eklendi. Gerçek Ayarlar penceresinde görüldü: [görüntü](../artifacts/desktop-review/reported-settings-icons.png). |
| Hover sırasında pencere simgeleri kayboluyor | Labwc hover efektini simgenin üzerine çiziyor; opak tema rengi simgeyi kapatıyordu. Renk saydam yapıldı, desteklenmeyen düğmeye özel renk anahtarı kaldırıldı. QTerminal'in küçült/büyüt/kapat düğmeleri ayrı ayrı hover ile kontrol edildi: [küçült](../artifacts/desktop-review/reported-minimize-hover.png), [büyüt](../artifacts/desktop-review/reported-maximize-hover.png), [kapat](../artifacts/desktop-review/reported-close-hover.png). Ayarlar'ın kendi kapatma düğmesi de görüldü. |
| Hızlı ayarlar aşağıda açılıyor | Compositor panelin ayrılmış alanını zaten çıkarıyordu; 66 px ek üst boşluk 4 px yapıldı. Bildirim merkezi de aynı düzeltmeyi kullanır. Hızlı ayarlar üst panelin hemen altında açıldı: [görüntü](../artifacts/desktop-review/reported-quick-position.png). |
| First Setup normal başlatıcıda | Sabit ilk kurulum kısayolu kaldırıldı; kalan dört kısayol satırı doldurur. İlk kurulumun sistem hizmeti korunur, masaüstü kaydı zaten NoDisplay=true. [Görüntü](../artifacts/desktop-review/reported-launcher-no-setup.png). |
| Mağazada kırmızı hata yazısı | Kullanıcı uygulama adını/hata metnini hatırlamıyor; özgün hata belirlenemedi. Gerçek mağazadan GPIO Tools APT ile ve VLC 3.0.23 Flatpak ile başarıyla kuruldu. İkisi de yeniden açılan mağazada kurulu göründü: [görüntü](../artifacts/desktop-review/reported-store-installed.png). Hatanın kök nedeni giderilmiş sayılmıyor. |

Mağazaya canlı paket çıktısı ve kaydırılabilir, seçilebilir tam hata alanı eklendi,
derlendi ve kuruldu. VS Code indirmesinde yeni canlı çıktı alanı görüldü:
[indirme görüntüsü](../artifacts/desktop-review/reported-store-live-download.png).
VS Code kurulumu başlatıldı; son durumu doğrulanamadı.

Güncelleme: sonraki kullanıcı geçişi turunda kurulumun tamamlandığı doğrulandı;
açılış sorunu root oturumundan kaynaklanıyordu. Aşağıdaki son kontroller de
tamamlandı. Ayrıntı: [VS Code ve normal kullanıcı geçişi](vscode-normal-user.md).

Bu kurulu sürümün derlemesi ve CTest 2/2 geçti (26 model, 10 dosya arayüzü
kontrolü; kurulum/kapanış dahil). Sonrasında işlem sırasında boş kalan mağaza
düğmesine Working… metni eklendi ve canlı çıktı/başarısız indirme testleri
genişletildi. **Bu son iki kaynak değişikliğinin derlemesi/testi çalıştırılmadı:**
otomatik onay denetimi kullanım sınırı nedeniyle tamamlanamadı ve işlem
yürütülmeden reddedildi. Bu, değişikliklerin güvensiz bulunduğu anlamına gelmez.
Çalışan mağaza, bu son küçük değişikliklerden önce derlenmiş sürümdür.

Devam ederken önce şu kontroller tamamlanmalıdır:

```sh
cmake --build --preset dev -j 4
ctest --preset dev --output-on-failure
cmake --install build/dev
flatpak info --user com.visualstudio.code
```

Ardından devam eden paket işlemi yoksa mağaza yeniden açılmalı; Working…
düğmesi ve kontrollü başarısız indirmede tam hata alanı görsel olarak
doğrulanmalıdır. Gerçek kullanıcı hatası tekrar oluşursa yeni paneldeki metin
üzerinden nedenine yönelik düzeltme yapılmalıdır.
