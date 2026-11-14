# SPDX-License-Identifier: GPL-3.0-or-later
"""Per-device, one-time provisioning; no shared password or TLS private key."""
import getpass
import hashlib
import hmac
import json
import os
from pathlib import Path
import re
import secrets
import stat
import subprocess
import tempfile
import uuid

HOSTNAME = re.compile(r'[a-z0-9](?:[a-z0-9-]{0,61}[a-z0-9])?')


class Paths:
    def __init__(self, root=Path('/')):
        self.root = Path(root)
    def at(self, path): return self.root / path.lstrip('/')
    @property
    def complete(self): return self.at('/var/lib/fidan/firstboot-complete').is_file()
    @property
    def enabled(self): return self.at('/etc/fidan/remote-enabled').is_file()
    @property
    def provision(self): return self.at('/var/lib/fidan/provision.json')


def read_json(path, limit=16384):
    descriptor = os.open(path, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK)
    with os.fdopen(descriptor, 'r') as stream:
        info = os.fstat(stream.fileno())
        if not stat.S_ISREG(info.st_mode) or info.st_size > limit: raise ValueError('Invalid configuration file')
        return json.load(stream)


def write_private(path, value, mode=0o600):
    path.parent.mkdir(parents=True, exist_ok=True)
    descriptor, temporary = tempfile.mkstemp(prefix='.fidan-', dir=path.parent)
    try:
        with os.fdopen(descriptor, 'w') as stream:
            os.fchmod(stream.fileno(), mode); json.dump(value, stream); stream.flush(); os.fsync(stream.fileno())
        os.replace(temporary, path)
    finally:
        Path(temporary).unlink(missing_ok=True)


def password_hash(password, salt):
    return hashlib.scrypt(password.encode('utf-8'), salt=bytes.fromhex(salt), n=32768, r=8, p=1, dklen=32, maxmem=64*1024*1024).hex()


def verify_password(password, config):
    if not isinstance(password, str) or len(password) > 256: return False
    try: return hmac.compare_digest(password_hash(password, config['salt']), config['passwordHash'])
    except (ValueError, KeyError, TypeError): return False


def prepare(boot, password, hostname='fidan'):
    boot = Path(boot).resolve(strict=True)
    if not boot.is_dir() or not HOSTNAME.fullmatch(hostname): raise ValueError('Choose a boot partition directory and valid hostname')
    if not 12 <= len(password.encode('utf-8')) <= 63 or any(c in password for c in '\n\r\0'): raise ValueError('Use a setup password of 12–63 UTF-8 bytes')
    destination = boot/'fidan-setup.json'
    if destination.exists() or destination.is_symlink(): raise FileExistsError('Provisioning data already exists; remove it explicitly before preparing another device')
    salt = secrets.token_hex(16); ssid = 'Fidan-Setup-' + secrets.token_hex(3)
    config = {'schema': 1, 'hostname': hostname, 'salt': salt, 'passwordHash': password_hash(password, salt), 'ssid': ssid,
              'wifiPsk': hashlib.pbkdf2_hmac('sha1', password.encode(), ssid.encode(), 4096, 32).hex()}
    # The FAT partition cannot enforce Unix permissions. Only a salted verifier
    # and a WPA2 PSK are written; never the account or setup password itself.
    # O_EXCL also rejects existing symlinks. Windows does not expose O_NOFOLLOW;
    # preparation is deliberately usable there before inserting the card in a Pi.
    descriptor = os.open(destination, os.O_WRONLY | os.O_CREAT | os.O_EXCL | getattr(os, 'O_NOFOLLOW', 0), 0o600)
    try:
        with os.fdopen(descriptor, 'w') as stream:
            json.dump(config, stream); stream.flush(); os.fsync(stream.fileno())
    except Exception:
        destination.unlink(missing_ok=True)
        raise
    return {'hostname': hostname + '.local', 'ssid': ssid}


def validate(config):
    if config.get('schema') != 1 or not HOSTNAME.fullmatch(str(config.get('hostname', ''))): raise ValueError('Invalid provisioning identity')
    for key, length in (('salt', 32), ('passwordHash', 64), ('wifiPsk', 64)):
        if not re.fullmatch(r'[0-9a-f]{%d}' % length, str(config.get(key, ''))): raise ValueError('Invalid provisioning verifier')
    if not re.fullmatch(r'Fidan-Setup-[0-9a-f]{6}', str(config.get('ssid', ''))): raise ValueError('Invalid setup network')
    return config


def cleanup(paths=Paths(), run=subprocess.run):
    if not paths.complete: return
    profile = paths.at('/etc/NetworkManager/system-connections/fidan-setup.nmconnection')
    if profile.exists():
        run(['nmcli', 'connection', 'delete', 'fidan-setup'], timeout=20, check=False, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        profile.unlink(missing_ok=True)
    paths.provision.unlink(missing_ok=True)
    for boot in ('/boot/firmware', '/boot'):
        paths.at(boot+'/fidan-setup.json').unlink(missing_ok=True)


def bootstrap(paths=Paths(), run=subprocess.run):
    import pwd
    if paths.complete: cleanup(paths, run)
    try: gateway_gid = pwd.getpwnam('fidan-gateway').pw_gid
    except KeyError:
        if paths.root == Path('/'): raise RuntimeError('Install the fidan-remote package before provisioning')
        gateway_gid = os.getgid()
    incoming = next((paths.at(boot+'/fidan-setup.json') for boot in ('/boot/firmware', '/boot') if paths.at(boot+'/fidan-setup.json').exists()), None)
    if incoming and not paths.complete:
        config = validate(read_json(incoming)); write_private(paths.provision, config, 0o640)
        os.chown(paths.provision, os.geteuid(), gateway_gid)
        run(['hostnamectl', 'set-hostname', config['hostname']], check=True, timeout=20)
        incoming.unlink()
    if not paths.provision.exists() and not paths.enabled: return
    tls = paths.at('/var/lib/fidan/tls'); tls.mkdir(parents=True, exist_ok=True, mode=0o750)
    os.chown(tls, os.geteuid(), gateway_gid); tls.chmod(0o750)
    if not (tls/'key.pem').exists() or not (tls/'cert.pem').exists():
        hostname = read_json(paths.provision)['hostname'] if paths.provision.exists() else 'fidan'
        run(['openssl', 'req', '-x509', '-newkey', 'rsa:3072', '-sha256', '-nodes', '-days', '3650', '-subj', '/CN='+hostname+'.local',
             '-addext', 'subjectAltName=DNS:fidan.local,DNS:'+hostname+'.local,IP:192.168.50.1', '-keyout', str(tls/'key.pem'), '-out', str(tls/'cert.pem')],
             check=True, timeout=60, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        for filename, mode in (('key.pem', 0o640), ('cert.pem', 0o644)):
            os.chown(tls/filename, os.geteuid(), gateway_gid); (tls/filename).chmod(mode)
    if paths.complete or not paths.provision.exists(): return
    status = run(['nmcli', '-t', '-f', 'TYPE,STATE', 'device', 'status'], capture_output=True, text=True, timeout=15, check=False)
    if any(line in ('ethernet:connected', 'wifi:connected') for line in status.stdout.splitlines()): return
    config = validate(read_json(paths.provision))
    profile = paths.at('/etc/NetworkManager/system-connections/fidan-setup.nmconnection')
    profile.parent.mkdir(parents=True, exist_ok=True)
    data = ('[connection]\nid=fidan-setup\nuuid='+str(uuid.uuid4())+'\ntype=wifi\nautoconnect=false\n'
            '[wifi]\nmode=ap\nssid='+config['ssid']+'\n[wifi-security]\nkey-mgmt=wpa-psk\npsk='+config['wifiPsk']+
            '\n[ipv4]\nmethod=shared\naddress1=192.168.50.1/24\n[ipv6]\nmethod=disabled\n')
    descriptor = os.open(profile, os.O_WRONLY | os.O_CREAT | os.O_TRUNC | os.O_NOFOLLOW, 0o600)
    with os.fdopen(descriptor, 'w') as stream: stream.write(data)
    run(['nmcli', 'connection', 'reload'], check=True, timeout=20)
    run(['nmcli', '--wait', '20', 'connection', 'up', 'fidan-setup'], check=False, timeout=25, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
