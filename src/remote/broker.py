# SPDX-License-Identifier: GPL-3.0-or-later
"""Local PAM/session broker. Only the unprivileged gateway can request a desktop FD."""
import array
import json
import os
from pathlib import Path
import pwd
import re
import secrets
import socket
import socketserver
import stat
import struct
import subprocess
import threading
import time
import traceback
from provision import Paths, read_json, verify_password

USER = re.compile(r'[a-z][a-z0-9_-]{1,30}')


def authenticate(username, password):
    import PAM
    if not isinstance(username, str) or not USER.fullmatch(username) or not isinstance(password, str) or not 1 <= len(password) <= 256 or '\0' in password:
        return None
    try:
        account = pwd.getpwnam(username)
        if account.pw_uid < 1000 or account.pw_shell.endswith(('nologin', 'false')): return None
    except KeyError: return None
    def conversation(auth, queries, data):
        responses = []
        for prompt, kind in queries:
            if kind == PAM.PAM_PROMPT_ECHO_ON: responses.append((username, 0))
            elif kind == PAM.PAM_PROMPT_ECHO_OFF: responses.append((password, 0))
            elif kind in (PAM.PAM_ERROR_MSG, PAM.PAM_TEXT_INFO): responses.append(('', 0))
            else: return None
        return responses
    auth = PAM.pam(); auth.start('fidan-remote'); auth.set_item(PAM.PAM_USER, username); auth.set_item(PAM.PAM_CONV, conversation)
    try:
        auth.authenticate(); auth.acct_mgmt()
        return account.pw_uid
    except PAM.error: return None
    finally:
        auth.set_item(PAM.PAM_CONV, lambda *args: None)


def user_environment(account, wayland=None):
    env = {'PATH': '/usr/bin:/bin', 'HOME': account.pw_dir, 'USER': account.pw_name, 'LOGNAME': account.pw_name,
           'XDG_RUNTIME_DIR': '/run/user/'+str(account.pw_uid), 'XDG_SESSION_TYPE': 'wayland', 'XDG_CURRENT_DESKTOP': 'Fidan'}
    if wayland: env['WAYLAND_DISPLAY'] = wayland
    return env


def find_wayland(uid):
    runtime = Path('/run/user')/str(uid)
    try:
        if runtime.is_symlink() or runtime.stat().st_uid != uid or runtime.stat().st_mode & 0o077: return None
        for entry in sorted(runtime.glob('wayland-*')):
            info = entry.lstat()
            if info.st_uid == uid and stat.S_ISSOCK(info.st_mode): return entry.name
    except OSError: return None
    return None


_capture_lock = threading.Lock()


def setup_diagnostics(uid, run=None):
    """Read only a fixed, bounded set of setup/capture records. Never follow logs."""
    run = run or subprocess.run
    units = ['fidan-firstboot-session.service', 'fidan-firstbootd.service',
             'fidan-remote-broker.service', 'fidan-gateway.service',
             'fidan-remote-output-' + str(uid) + '.service',
             'fidan-remote-capture-' + str(uid) + '.service']
    def collect(command, limit):
        try:
            result = run(command, capture_output=True, timeout=5, check=False)
            value = result.stdout
            if isinstance(value, str): value = value.encode()
            lines = value[:limit].decode('utf-8', errors='replace').splitlines()
            return '\n'.join('[redacted]' if re.search(r'password|passwd|token|cookie|authorization|secret|wifiPsk', line, re.I) else line for line in lines)
        except (OSError, subprocess.SubprocessError):
            return 'Diagnostic command unavailable or timed out.'
    properties = ['Id', 'LoadState', 'ActiveState', 'SubState', 'Result', 'ExecMainStatus', 'NRestarts']
    status = collect(['systemctl', 'show', '--no-pager', *['--property='+p for p in properties], *units], 4096)
    journal = collect(['journalctl', '-b', '--no-pager', '-n', '60', '-o', 'short-monotonic',
                       *[arg for unit in units for arg in ('-u', unit)]], 20000)
    return {'schema': 1, 'scope': 'setup-connection', 'captureLauncher':'systemd-user-unit-v2', 'follow': False, 'status': status, 'journal': journal,
            'inputDeviceCount': len(list(Path('/dev/input').glob('event*')))}


def capture_socket(uid, setup=False):
    account = pwd.getpwuid(uid)
    with _capture_lock:
        wayland = find_wayland(uid)
        if not wayland and setup:
            # Share the same Labwc + shell + native installer as the local screen.
            # Never start a normal user desktop under the setup account.
            if account.pw_name != 'fidan-setup': raise PermissionError('Invalid setup account')
            subprocess.run(['systemctl', 'start', 'fidan-firstboot-session.service'], check=True, timeout=15,
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            for attempt in range(100):
                wayland = find_wayland(uid)
                if wayland: break
                time.sleep(0.1)
            if not wayland: raise RuntimeError('The setup desktop could not start; check the device journal')
        if not wayland:
            env = user_environment(account)
            env.update(WLR_BACKENDS='headless', WLR_HEADLESS_OUTPUTS='1', WLR_RENDERER='pixman')
            command = ['systemd-run', '--quiet', '--collect', '--unit=fidan-remote-session-'+str(uid), '--uid='+account.pw_name,
                       '--property=PAMName=fidan-remote', '--property=Type=exec']
            command += ['--setenv='+key+'='+value for key, value in env.items()]
            subprocess.run(command + ['--', '/usr/bin/dbus-run-session', '/usr/bin/fidan-session'], check=True, timeout=15,
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            for attempt in range(100):
                wayland = find_wayland(uid)
                if wayland: break
                time.sleep(0.1)
            if not wayland: raise RuntimeError('The desktop could not start; check the device journal')
        path = Path('/run/user')/str(uid)/'fidan-remote-ws.sock'
        def connect():
            info = path.lstat()
            if not stat.S_ISSOCK(info.st_mode) or info.st_uid != uid or info.st_mode & 0o077: raise RuntimeError('Untrusted desktop socket')
            stream = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM); stream.settimeout(10)
            try:
                stream.connect(str(path))
                peer = struct.unpack('3i', stream.getsockopt(socket.SOL_SOCKET, socket.SO_PEERCRED, 12))
                if peer[1] != uid: raise RuntimeError('Wrong desktop socket owner')
                return stream
            except Exception: stream.close(); raise
        try: return connect()
        except FileNotFoundError: pass
        env = user_environment(account, wayland)
        # The broker's sandbox can deny setuid/setgroups in a forked child.
        # Ask PID 1 to create the fixed, unprivileged helper, just as for capture.
        prepare = ['systemd-run', '--quiet', '--wait', '--pipe', '--collect',
                   '--unit=fidan-remote-output-'+str(uid), '--uid='+account.pw_name,
                   '--property=NoNewPrivileges=yes', '--property=RestrictAddressFamilies=AF_UNIX']
        prepare += ['--setenv='+key+'='+value for key, value in env.items()]
        subprocess.run(prepare + ['--', '/usr/bin/fidan-remote-session', 'prepare-output'], check=True, timeout=15,
                       stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        command = ['systemd-run', '--quiet', '--collect', '--unit=fidan-remote-capture-'+str(uid), '--uid='+account.pw_name,
                   '--property=UMask=0077', '--property=NoNewPrivileges=yes', '--property=RestrictAddressFamilies=AF_UNIX',
                   '--property=MemoryMax=256M']
        command += ['--setenv='+key+'='+value for key, value in env.items()]
        subprocess.run(command + ['--', '/usr/bin/fidan-remote-session', 'capture'], check=True, timeout=15,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        for attempt in range(100):
            try: return connect()
            except FileNotFoundError: time.sleep(0.1)
        raise RuntimeError('The desktop capture service did not start')


class Broker:
    def __init__(self, authenticate_user=authenticate, capture=capture_socket, paths=None, setup_account=None):
        self.authenticate = authenticate_user; self.capture = capture
        self.paths = paths or Paths(); self.setup_account = setup_account or (lambda: pwd.getpwnam('fidan-setup'))
        self.sessions = {}; self.failures = []; self.lock = threading.Lock()
        self.capture_errors = {}
    def login(self, username, password, setup=False):
        with self.lock:
            now = time.monotonic(); self.failures = [t for t in self.failures if now-t < 60]
            self.sessions = {key: value for key, value in self.sessions.items() if value[1] > now}
            self.capture_errors = {key:value for key,value in self.capture_errors.items() if key in self.sessions}
            if len(self.failures) >= 8 or len(self.sessions) >= 32: raise PermissionError('Too many attempts; retry in a minute')
            self.failures.append(now)
            if setup:
                if self.paths.complete or not self.paths.provision.is_file(): raise PermissionError('Setup is unavailable')
                if not verify_password(password, read_json(self.paths.provision)): raise PermissionError('Sign-in failed')
                account = self.setup_account()
                if account.pw_name != 'fidan-setup' or account.pw_uid <= 0: raise PermissionError('Invalid setup account')
                uid = account.pw_uid
            else:
                if not self.paths.complete or not self.paths.enabled: raise PermissionError('Desktop access is disabled')
                uid = self.authenticate(username, password)
                if uid is None or uid < 1000: raise PermissionError('Sign-in failed')
            token = secrets.token_urlsafe(32); self.sessions[token] = (uid, now+1200, 'setup' if setup else 'desktop')
            return token
    def connect(self, token):
        if not isinstance(token, str): raise ValueError('Invalid session token')
        with self.lock: session = self.sessions.get(token)
        if session is None or session[1] <= time.monotonic(): raise PermissionError('Sign in again')
        if session[2] == 'setup':
            if self.paths.complete or not self.paths.provision.is_file(): raise PermissionError('Setup is complete')
            try:
                stream = self.capture(session[0], setup=True)
                with self.lock: self.capture_errors.pop(token, None)
                return stream
            except (OSError, ValueError, RuntimeError, subprocess.SubprocessError) as error:
                message = str(error)[:1500]
                if isinstance(error,subprocess.CalledProcessError) and error.stderr:
                    detail=error.stderr.decode('utf-8',errors='replace') if isinstance(error.stderr,bytes) else error.stderr
                    message=('Output helper failed: '+detail)[-1500:]
                if re.search(r'password|passwd|token|cookie|authorization|secret', message, re.I): message = '[redacted]'
                trace=[{'file':Path(frame.filename).name,'line':frame.lineno,'function':frame.name} for frame in traceback.extract_tb(error.__traceback__)[-6:]]
                with self.lock: self.capture_errors[token] = {'type':type(error).__name__, 'message':message,'trace':trace}
                raise
        if not self.paths.complete or not self.paths.enabled: raise PermissionError('Desktop access is disabled')
        return self.capture(session[0])
    def logout(self, token):
        if not isinstance(token, str): raise ValueError('Invalid session token')
        with self.lock:
            self.sessions.pop(token, None); self.capture_errors.pop(token, None)

    def diagnostics(self, token):
        if not isinstance(token, str): raise PermissionError('Sign in again')
        with self.lock: session = self.sessions.get(token)
        if session is None or session[1] <= time.monotonic() or session[2] != 'setup':
            raise PermissionError('Setup diagnostics require a setup session')
        if self.paths.complete or not self.paths.provision.is_file(): raise PermissionError('Setup is complete')
        report = setup_diagnostics(session[0])
        with self.lock: report['lastCaptureFailure'] = self.capture_errors.get(token)
        return report


class BrokerHandler(socketserver.StreamRequestHandler):
    def handle(self):
        self.connection.settimeout(15)
        peer = struct.unpack('3i', self.connection.getsockopt(socket.SOL_SOCKET, socket.SO_PEERCRED, 12))
        if peer[1] != self.server.gateway_uid: return
        try:
            data = self.rfile.readline(4097)
            if len(data) > 4096 or not data.endswith(b'\n'): raise ValueError('Invalid request')
            request = json.loads(data)
            if not isinstance(request, dict): raise ValueError('Invalid request')
            if request.get('command') in ('login', 'setup-login'):
                token = self.server.broker.login(request.get('username'), request.get('password'), setup=request['command']=='setup-login')
                self.wfile.write(json.dumps({'ok': True, 'token': token}).encode()+b'\n'); self.wfile.flush()
            elif request.get('command') == 'logout':
                self.server.broker.logout(request.get('token')); self.wfile.write(b'{"ok":true}\n')
            elif request.get('command') == 'connect':
                with self.server.broker.connect(request.get('token')) as desktop:
                    self.wfile.write(b'{"ok":true}\n'); self.wfile.flush()
                    self.connection.sendmsg([b'F'], [(socket.SOL_SOCKET, socket.SCM_RIGHTS, array.array('i', [desktop.fileno()]))])
            elif request.get('command') == 'diagnostics':
                report = self.server.broker.diagnostics(request.get('token'))
                self.wfile.write(json.dumps({'ok': True, 'report': report}, ensure_ascii=False).encode()+b'\n'); self.wfile.flush()
            else: raise ValueError('Unknown request')
        except (OSError, ValueError, RuntimeError, subprocess.SubprocessError) as error:
            self.wfile.write(json.dumps({'ok': False, 'error': str(error)}).encode()+b'\n')


class BrokerServer(socketserver.ThreadingUnixStreamServer):
    daemon_threads = True
    def __init__(self, path, gateway_uid, broker=None):
        self.gateway_uid = gateway_uid; self.broker = broker or Broker(); self.slots = threading.BoundedSemaphore(8)
        super().__init__(str(path), BrokerHandler)
    def process_request(self, request, address):
        if not self.slots.acquire(False): request.close(); return
        super().process_request(request, address)
    def process_request_thread(self, request, address):
        try: super().process_request_thread(request, address)
        finally: self.slots.release()
    def handle_error(self, request, address): pass # Never log request bodies or passwords.


def request(command, path='/run/fidan-remote/broker.sock', receive_fd=False):
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as stream:
        stream.settimeout(40); stream.connect(str(path)); stream.sendall(json.dumps(command).encode()+b'\n')
        data = bytearray()
        reply_limit = 65536 if command.get('command') == 'diagnostics' else 4096
        while len(data) <= reply_limit:
            byte = stream.recv(1)
            if not byte: raise ConnectionError('The session broker disconnected')
            if byte == b'\n': break
            data.extend(byte)
        else: raise ValueError('Invalid broker reply')
        result = json.loads(data)
        if not result.get('ok'): raise PermissionError(result.get('error', 'Sign-in failed'))
        if not receive_fd: return result
        payload, controls, flags, address = stream.recvmsg(1, socket.CMSG_SPACE(array.array('i').itemsize))
        descriptors = array.array('i')
        for level, kind, value in controls:
            if level == socket.SOL_SOCKET and kind == socket.SCM_RIGHTS: descriptors.frombytes(value[:len(value)-len(value)%descriptors.itemsize])
        if payload != b'F' or len(descriptors) != 1:
            for fd in descriptors: os.close(fd)
            raise ValueError('Invalid desktop descriptor')
        return socket.socket(fileno=descriptors[0])
