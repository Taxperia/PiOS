# VirtualBox ile PiOS'u sıfırdan denemek

PiOS şu anda Debian 13 üzerinde çalışan masaüstü paketleri olarak dağıtılıyor.
GitHub Releases'te VirtualBox'a takılabilecek bir PiOS ISO/OVA dosyası yok.
`image/` altındaki imaj tarifi Raspberry Pi 5 içindir; PC sanal makinesinde
başlatılmaz. Windows bilgisayarında **Debian 13 amd64** sanal makinesi oluşturup
PiOS'u onun içine kurabilirsin. Mevcut Windows ve WSL dosyaların bu kurulumdan
bağımsız kalır. Aşağıdaki terminal komutlarının tamamı **sanal Debian içinde**
çalıştırılmalıdır.

## 1. Sanal makineyi oluştur

1. [VirtualBox](https://www.virtualbox.org/wiki/Downloads) ve
   [Debian'ın resmî netinst ISO'sunu](https://www.debian.org/distrib/netinst) indir.
   Bu Intel/AMD Windows bilgisayarı için `amd64` seç.
2. VirtualBox → **Yeni**: ad `PiOS Test`, tür Linux, sürüm Debian (64-bit).
   ISO'yu seç; elle kurulum için **Skip Unattended Installation** seçeneğini aç.
3. Başlangıç önerisi: **2 işlemci, 4 GB RAM, 40 GB dinamik VDI**. Ana bilgisayara
   yeterli RAM bırak. VDI için boş alanı bol olan bir sürücü seç; dinamik disk
   kullanıldıkça büyür.
4. Ekran: **VMSVGA**, 128 MB görüntü belleği. İlk denemede 3D kapalı başlayabilirsin;
   Wayland'ın yazılımsal çizimi daha yavaş olabilir. Grafik sorunu yaşarsan sanal
   makineyi kapatıp 3D seçeneğini ve Guest Additions'ı ayrıca değerlendirebilirsin.
   Ağ için **NAT** yeterli.
5. Başlat → Debian **Graphical install**. Dilini ve klavyeni seç. Normal bir
   yönetici hesabı oluştur; root parolasını boş bırakırsan Debian normal kullanıcıyı
   sudo grubuna ekler. Bölümlemede yalnızca **sanal 40 GB disk** görünmelidir.
6. Yazılım seçiminde **masaüstü ortamı/GNOME seçimini kaldır**, **standard system
   utilities** seçili kalsın. PiOS kendi masaüstünü ve giriş ekranını kuracak.
7. Debian kurulunca ISO'yu sanal optik sürücüden çıkar ve Debian'a giriş yap.

Oracle'ın [sanal makine kılavuzu](https://www.virtualbox.org/manual/topics/BasicConcepts.html)
ve [Guest Additions kılavuzu](https://docs.oracle.com/en/virtualization/virtualbox/7.2/user/guestadditions.html)
ekran, sanal donanım ve misafir sürücü ayrıntılarını açıklar. PiOS'un fiziksel
Raspberry Pi ve VirtualBox donanımları aynı değildir; Wi-Fi/Bluetooth yerine
sanal Ethernet gibi aygıtlar göreceksin. Bu adımlar gerçek bir VirtualBox
kurulumunda henüz uçtan uca doğrulanmadı.

## 2. PiOS paketlerini kur

Sanal Debian terminalinde:

```sh
sudo apt update
sudo apt install git ca-certificates
git clone https://github.com/Taxperia/PiOS.git
cd PiOS
./scripts/setup-debian-dev
./scripts/build-debs
```

Derleme bilgisayara göre zaman alır. Paketleri kur:

```sh
find dist -maxdepth 1 -type f -name '*.deb' ! -name '*-dbgsym_*' \
  \( -name '*_amd64.deb' -o -name '*_all.deb' \) -print0 \
  | xargs -0 -r sudo apt install -y
```

Bu yol GitHub'a gönderilmiş kaynak sürümünü derler. Yerel geliştirmede eklenmiş
ama henüz GitHub'a gönderilmemiş uygulamalar klonlanan sürümde bulunmaz. Son
yayımlanmış alpha.3 sürümü PDF/hesap makinesi/saat/takvim eklenmeden önceki sürümdür.
Yeni uygulamaları denemek için bu değişiklikleri içeren kaynak/paketler gerekir.

Alternatif olarak [GitHub Releases](https://github.com/Taxperia/PiOS/releases)
sayfasından aynı sürümün **tüm `_amd64.deb` ve `_all.deb` paketlerini** indirip
yalnızca onlara ayrılmış bir klasörde `sudo apt install ./*.deb` kullanabilirsin.
`arm64` paketlerini alma. İmza ve SHA-256 doğrulaması için [release belgesine](release.md)
bak. Windows'ta `.deb` dosyasına çift tıklamak sanal Debian'a kurulum yapmaz.

## 3. İlk açılış deneyimini etkinleştir

Bu bölüm yalnızca yeni oluşturduğun **sanal Debian** içindir. Açık geliştirme/WSL
oturumunda çalıştırma. Paket kurulumu, kurulu başka bir giriş yöneticisinin yerini
kendiliğinden değiştirmez. Yukarıdaki gibi masaüstüsüz Debian kurduysan:

```sh
sudo install -m 0644 /etc/greetd/fidan.toml /etc/greetd/config.toml
sudo mkdir -p /var/lib/fidan
sudo systemctl disable greetd.service
sudo systemctl enable fidan-firstbootd.service fidan-firstboot-session.service
sudo systemctl set-default graphical.target
sudo reboot
```

İlk kurulum ekranında dil, kullanıcı, parola ve gizlilik seçimlerini yap. Debian
kurulumunda açtığın yönetici hesabından **farklı bir kullanıcı adı** seç; örneğin
Debian'da `vmadmin`, PiOS ilk kurulumunda `fidan`. İlk kurulum tamamlanınca sistem
yeniden başlar ve PiOS giriş ekranı gelir. Seçtiğin parolayı burada kullanırsın.

İlk kurulum sorun çıkarırsa sanal makinede Ctrl+Alt+F2 ile metin konsoluna geçip
Debian'daki yönetici hesabınla giriş yap. VirtualBox bu tuşları ana bilgisayara
gönderiyorsa menüde **Input → Keyboard** veya sanal klavyeyi kullan. Teşhis:

```sh
systemctl status fidan-firstboot-session fidan-firstbootd greetd
journalctl -b -u fidan-firstboot-session -u fidan-firstbootd -u greetd
```

Sadece mevcut Debian hesabıyla masaüstüne girmek istersen ilk kurulum servisini
devre dışı bırakıp greetd'yi etkinleştirebilirsin. Bu yol ilk kurulum deneyimini atlar:

```sh
sudo systemctl disable --now fidan-firstboot-session.service
sudo systemctl enable greetd.service
sudo reboot
```

## 4. Her seferinde baştan dene

VirtualBox'ta sanal makine kapalıyken, PiOS paketleri kurulduktan ama ilk kurulum
tamamlanmadan önce **Snapshot → Take** ile `İlk kurulum öncesi` anlık görüntüsü al.
Denemeden sonra bu görüntüyü geri yükleyerek aynı temiz başlangıca dönebilirsin.
Geri yüklemek o görüntüden sonra **sanal makinede** oluşturduğun dosyaları da geri
alır; saklayacağın dosyaları önce dışarı kopyala.
