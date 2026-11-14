# SPDX-License-Identifier: GPL-3.0-or-later
"""Optional, loopback-only xrdp maintenance desktop. No service runs until opted in."""
import os
from pathlib import Path
import pwd
import grp
import re
import shutil
import stat
import subprocess
import tempfile


def ini_values(text, section, values):
    """Replace all occurrences of selected keys, preserving repeated Xorg param keys."""
    output = []; current = ''; found = False
    keys = {key.lower() for key in values}
    def insert():
        output.extend(key + '=' + value for key, value in values.items())
    for line in text.splitlines():
        header = re.fullmatch(r'\s*\[([^]]+)\]\s*', line)
        if header:
            if current == section.lower(): insert()
            current = header[1].lower()
            if current == section.lower():
                if found: raise ValueError('Duplicate configuration section: ' + section)
                found = True
        if current == section.lower() and '=' in line and not line.lstrip().startswith(('#', ';')):
            if line.split('=', 1)[0].strip().lower() in keys: continue
        output.append(line)
    if current == section.lower(): insert()
    if not found: output.extend(['['+section+']']); insert()
    return '\n'.join(output) + '\n'


def write_config(path, text):
    """The config directory is root-owned; preserve the administrator's original once."""
    info = path.lstat()
    if not stat.S_ISREG(info.st_mode) or info.st_uid != 0 or info.st_mode & 0o022:
        raise ValueError('Untrusted xrdp configuration: ' + str(path))
    backup = path.with_name(path.name + '.before-fidan')
    if not backup.exists():
        with backup.open('x') as stream:
            os.chmod(backup, 0o600); stream.write(path.read_text())
    descriptor, temporary = tempfile.mkstemp(prefix='.fidan-', dir=path.parent)
    try:
        with os.fdopen(descriptor, 'w') as stream:
            os.fchmod(stream.fileno(), stat.S_IMODE(info.st_mode))
            os.fchown(stream.fileno(), info.st_uid, info.st_gid)
            stream.write(text); stream.flush(); os.fsync(stream.fileno())
        os.replace(temporary, path)
    finally: Path(temporary).unlink(missing_ok=True)


def enable(username, root=Path('/'), run=subprocess.run):
    if not re.fullmatch(r'[a-z][a-z0-9_-]{1,30}', username): raise ValueError('Choose a local username')
    account = pwd.getpwnam(username)
    if account.pw_uid < 1000 or account.pw_uid >= 65534 or account.pw_shell.endswith(('nologin', 'false')):
        raise ValueError('Choose a regular local account')
    for command in ('xrdp', 'Xorg', 'openbox', 'pcmanfm-qt', 'qterminal', 'openssl'):
        if not shutil.which(command): raise ValueError('Missing '+command+'; follow docs/rdp.md to install the optional packages')
    # A partial configuration must never leave the stock public listener running.
    run(['systemctl', 'stop', 'xrdp.service', 'xrdp-sesman.service'], check=True, timeout=30)
    config = root/'etc/xrdp/xrdp.ini'; sesman = root/'etc/xrdp/sesman.ini'
    tls = root/'var/lib/fidan/rdp'; tls.mkdir(mode=0o750, parents=True, exist_ok=True)
    xrdp_group = grp.getgrnam('xrdp').gr_gid
    os.chown(tls, 0, xrdp_group); tls.chmod(0o750)
    if not (tls/'key.pem').exists() or not (tls/'cert.pem').exists():
        run(['openssl', 'req', '-x509', '-newkey', 'rsa:3072', '-sha256', '-nodes', '-days', '825',
             '-subj', '/CN=PiOS RDP', '-keyout', str(tls/'key.pem'), '-out', str(tls/'cert.pem')],
            check=True, timeout=60, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    for name, mode in (('key.pem', 0o640), ('cert.pem', 0o644)):
        os.chown(tls/name, 0, xrdp_group); (tls/name).chmod(mode)
    write_config(config, ini_values(config.read_text(), 'Globals', {
        'port': 'tcp://127.0.0.1:3389', 'security_layer': 'tls', 'ssl_protocols': 'TLSv1.2, TLSv1.3',
        'certificate': '/var/lib/fidan/rdp/cert.pem', 'key_file': '/var/lib/fidan/rdp/key.pem', 'max_bpp': '24'}))
    text = ini_values(sesman.read_text(), 'Globals', {'EnableUserWindowManager': 'false', 'DefaultWindowManager': '/usr/bin/fidan-rdp-session'})
    text = ini_values(text, 'Security', {'AllowRootLogin': 'false', 'MaxLoginRetry': '3', 'TerminalServerUsers': 'fidan-rdp-users', 'AlwaysGroupCheck': 'true'})
    text = ini_values(text, 'Sessions', {'MaxSessions': '1', 'KillDisconnected': 'true', 'DisconnectedTimeLimit': '60', 'IdleTimeLimit': '1200'})
    write_config(sesman, text)
    run(['groupadd', '--force', 'fidan-rdp-users'], check=True, timeout=15)
    run(['usermod', '--append', '--groups', 'fidan-rdp-users', username], check=True, timeout=15)
    run(['systemctl', 'unmask', 'xrdp.service', 'xrdp-sesman.service'], check=True, timeout=15)
    run(['systemctl', 'enable', '--now', 'xrdp-sesman.service', 'xrdp.service'], check=True, timeout=30)
