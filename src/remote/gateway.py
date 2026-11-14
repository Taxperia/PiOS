# SPDX-License-Identifier: GPL-3.0-or-later
"""Bounded HTTPS portal; provisioning and authenticated noVNC over private Unix IPC."""
from http import HTTPStatus
from http.cookies import SimpleCookie, CookieError
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import ipaddress
import json
import mimetypes
from pathlib import Path
import secrets
import select
import socket
import ssl
import threading
import time
from urllib.parse import unquote, urlsplit
from provision import Paths
import broker


class Sessions:
    def __init__(self): self.items = {}; self.attempts = {}; self.lock = threading.Lock(); self.password_slots = threading.BoundedSemaphore(2)
    def attempt(self, ip):
        with self.lock:
            now = time.monotonic()
            self.attempts = {key: [t for t in times if now-t < 60] for key, times in self.attempts.items() if times and now-times[-1] < 60}
            times = self.attempts.setdefault(ip, [])
            if len(times) >= 5 or len(self.attempts) > 256: return False
            times.append(now); return True
    def create(self, role, token=None):
        with self.lock:
            now = time.monotonic(); self.items = {key: value for key, value in self.items.items() if value['expires'] > now}
            if len(self.items) >= 32: raise PermissionError('Too many sessions')
            sid = secrets.token_urlsafe(32); session = {'role': role, 'token': token, 'csrf': secrets.token_urlsafe(32), 'expires': now+broker.SESSION_IDLE_SECONDS, 'until':now+broker.SESSION_MAX_SECONDS}
            self.items[sid] = session
            return sid, session
    def get(self, cookie):
        try:
            values = SimpleCookie(cookie); sid = values['__Host-pios_session'].value
        except (KeyError, ValueError, CookieError): return None
        with self.lock: session = self.items.get(sid)
        return session if session and min(session['expires'],session['until']) > time.monotonic() else None
    def renew(self,cookie):
        try:sid=SimpleCookie(cookie)['__Host-pios_session'].value
        except (KeyError,ValueError,CookieError):raise PermissionError('Sign in again') from None
        with self.lock:
            now=time.monotonic();session=self.items.get(sid)
            if not session or min(session['expires'],session['until'])<=now:raise PermissionError('Sign in again')
            session['expires']=min(now+broker.SESSION_IDLE_SECONDS,session['until'])
        return sid
    def remove(self, cookie):
        try: sid = SimpleCookie(cookie)['__Host-pios_session'].value
        except (KeyError, ValueError, CookieError): return
        with self.lock: self.items.pop(sid, None)


NATIVE_SETUP_DESKTOP = True


class Gateway(ThreadingHTTPServer):
    daemon_threads = True
    def __init__(self, address, tls, paths=Paths(), web_root=None, novnc_root=Path('/usr/share/novnc'), ipc=broker.request):
        self.tls = tls; self.paths = paths; self.sessions = Sessions(); self.ipc = ipc
        self.web_root = Path(web_root or '/usr/share/fidan/remote'); self.novnc_root = Path(novnc_root)
        self.wallpaper = Path('/usr/share/backgrounds/fidan/fidan-mountain-dawn.png')
        self.slots = threading.BoundedSemaphore(16)
        super().__init__(address, Handler)
    def process_request(self, request, address):
        if not self.slots.acquire(False): request.close(); return
        super().process_request(request, address)
    def process_request_thread(self, request, address):
        try: super().process_request_thread(request, address)
        finally: self.slots.release()
    def handle_error(self, request, address): pass


class Handler(BaseHTTPRequestHandler):
    server_version = 'PiOS'; sys_version = ''; protocol_version = 'HTTP/1.0'
    def setup(self):
        self.request.settimeout(10)
        self.request = self.server.tls.wrap_socket(self.request, server_side=True)
        super().setup()
    def log_message(self, *args): pass
    def finish(self):
        try: super().finish()
        finally: self.connection.close()
    def valid_origin(self, required=False):
        origin = self.headers.get('Origin')
        return origin == 'https://' + self.headers.get('Host','') if origin or required else True
    def valid_host(self):
        host = self.headers.get('Host','')
        if len(host) > 256: return False
        try:
            parsed = urlsplit('//'+host)
            if parsed.username or parsed.password or parsed.path or parsed.query or parsed.fragment or parsed.port not in (None, self.server.server_port): return False
            hostname = parsed.hostname
            if hostname in {'fidan.local', socket.gethostname()+'.local'}: return True
            return ipaddress.ip_address(hostname) == ipaddress.ip_address(self.connection.getsockname()[0])
        except (ValueError, TypeError): return False
    def allowed(self):
        try:
            ip = ipaddress.ip_address(self.client_address[0]); ip = getattr(ip, 'ipv4_mapped', None) or ip
            local = (ip.is_private or ip.is_loopback or ip.is_link_local) and not ip.is_multicast and not ip.is_unspecified
            return local and self.valid_host() and (self.server.paths.enabled or not self.server.paths.complete and self.server.paths.provision.is_file())
        except ValueError: return False
    def reply(self, status, value, cookie=None):
        body = json.dumps(value, ensure_ascii=False).encode()
        self.send_response(status); self.common_headers(); self.send_header('Content-Type','application/json; charset=utf-8')
        self.send_header('Content-Length',str(len(body)))
        if cookie is not None: self.send_header('Set-Cookie', '__Host-pios_session='+cookie+'; Path=/; Secure; HttpOnly; SameSite=Strict; Max-Age='+(str(broker.SESSION_IDLE_SECONDS) if cookie else '0'))
        self.end_headers(); self.wfile.write(body)
    def common_headers(self):
        self.send_header('Cache-Control','no-store'); self.send_header('X-Content-Type-Options','nosniff'); self.send_header('X-Frame-Options','DENY')
        self.send_header('Referrer-Policy','no-referrer')
        self.send_header('Content-Security-Policy', "default-src 'self'; script-src 'self'; style-src 'self' 'unsafe-inline'; img-src 'self' data:; connect-src 'self' wss:; frame-ancestors 'none'; base-uri 'none'; form-action 'self'")
    def session(self):
        session = self.server.sessions.get(self.headers.get('Cookie',''))
        if session and session['role'] == 'setup' and self.server.paths.complete: return None
        if session and session['role'] == 'desktop' and not self.server.paths.enabled: return None
        return session
    def do_GET(self):
        if not self.allowed() or not self.valid_origin(): self.reply(403, {'error':'Access is disabled or the request origin is invalid'}); return
        path = urlsplit(self.path).path
        if path == '/api/mode': self.reply(200, {'mode':'desktop' if self.server.paths.complete else 'setup'}); return
        if path == '/api/state':
            session = self.session()
            if not session: self.reply(401, {'error':'Sign in again'}); return
            self.reply(200, {'role':session['role'], 'csrf':session['csrf'], 'mode':'desktop' if self.server.paths.complete else 'setup', 'sessionRenewal':True})
            return
        if path == '/api/diagnostics':
            session = self.session()
            if not session or session['role'] != 'setup': self.reply(403, {'error':'Setup sign-in required'}); return
            try:
                result = self.server.ipc({'command':'diagnostics', 'token':session['token']})
                self.reply(200, result['report'])
            except (OSError, ValueError, KeyError): self.reply(503, {'error':'The connection report is unavailable'})
            return
        if path == '/desktop-socket': self.desktop(); return
        if path == '/wallpaper.png':
            self.static(self.server.wallpaper.parent,self.server.wallpaper.name); return
        if path.startswith('/novnc/'):
            session = self.session()
            if not session or session['role'] not in ('setup', 'desktop'): self.reply(401, {'error':'Sign in first'}); return
            self.static(self.server.novnc_root, unquote(path[len('/novnc/'):]))
        elif path in ('/', '/ui.js', '/style.css'): self.static(self.server.web_root, 'index.html' if path=='/' else path[1:])
        else: self.reply(404, {'error':'Not found'})
    def static(self, root, relative):
        root = root.resolve(); candidate = (root/relative).resolve()
        if not candidate.is_relative_to(root) or not candidate.is_file() or candidate.stat().st_size > 8*1024*1024:
            self.reply(404, {'error':'Not found'}); return
        body = candidate.read_bytes(); self.send_response(200); self.common_headers()
        self.send_header('Content-Type', mimetypes.guess_type(str(candidate))[0] or 'application/octet-stream')
        self.send_header('Content-Length', str(len(body))); self.end_headers(); self.wfile.write(body)
    def do_POST(self):
        try:
            length = int(self.headers.get('Content-Length','0'))
            if not 1 <= length <= 8192: raise ValueError('Invalid request size')
            body = self.rfile.read(length)
            if not self.allowed() or not self.valid_origin(True): self.reply(403, {'error':'Invalid origin'}); return
            if self.headers.get('Content-Type','').split(';')[0] != 'application/json': raise ValueError('Invalid content type')
            data = json.loads(body)
            if not isinstance(data, dict): raise ValueError('Invalid request')
        except (ValueError, OSError): self.reply(400, {'error':'Invalid request'}); return
        path = urlsplit(self.path).path
        if path == '/api/login':
            if not self.server.sessions.attempt(self.client_address[0]) or not self.server.sessions.password_slots.acquire(False): self.reply(429, {'error':'Wait a minute before trying again'}); return
            try:
                if not self.server.paths.complete:
                    result = self.server.ipc({'command':'setup-login', 'password':data.get('password')})
                    sid, session = self.server.sessions.create('setup', result['token'])
                else:
                    result = self.server.ipc({'command':'login', 'username':data.get('username'), 'password':data.get('password')})
                    sid, session = self.server.sessions.create('desktop', result['token'])
                self.reply(200, {'role':session['role'], 'csrf':session['csrf']}, sid)
            except (OSError, ValueError, KeyError): self.reply(401, {'error':'Sign-in failed'})
            finally: self.server.sessions.password_slots.release()
            return
        session = self.session()
        if not session or not secrets.compare_digest(self.headers.get('X-CSRF-Token',''), session['csrf']): self.reply(403, {'error':'Sign in again'}); return
        if path == '/api/logout':
            if session['token']:
                try: self.server.ipc({'command':'logout','token':session['token']})
                except OSError: pass
            self.server.sessions.remove(self.headers.get('Cookie','')); self.reply(200, {'ok':True}, ''); return
        if path == '/api/keepalive':
            try:
                self.server.ipc({'command':'renew','token':session['token']})
                sid=self.server.sessions.renew(self.headers.get('Cookie',''))
                self.reply(200,{'ok':True},sid)
            except (OSError,ValueError): self.reply(401,{'error':'Sign in again'})
            return
        self.reply(403, {'error':'Not allowed'})
    def desktop(self):
        session = self.session()
        if not session or session['role'] not in ('setup', 'desktop') or not self.valid_origin(True): self.reply(403, {'error':'Sign in again'}); return
        if self.headers.get('Upgrade','').lower() != 'websocket' or self.headers.get('Sec-WebSocket-Version') != '13': self.reply(400, {'error':'WebSocket required'}); return
        key = self.headers.get('Sec-WebSocket-Key','')
        if not 1 <= len(key) <= 64: self.reply(400, {'error':'Invalid WebSocket key'}); return
        try:
            desktop = self.server.ipc({'command':'connect','token':session['token']}, receive_fd=True)
        except (OSError, ValueError): self.reply(503, {'error':'The desktop could not start. Check the device journal and retry.'}); return
        with desktop:
            desktop.settimeout(10)
            # The user's private websockify bridge performs WebSocket/RFB framing.
            # WayVNC's raw RFB listener is another private Unix socket, never TCP.
            headers = ['GET / HTTP/1.1', 'Host: localhost', 'Connection: Upgrade', 'Upgrade: websocket', 'Sec-WebSocket-Version: 13', 'Sec-WebSocket-Key: '+key]
            if self.headers.get('Sec-WebSocket-Protocol') == 'binary': headers.append('Sec-WebSocket-Protocol: binary')
            desktop.sendall(('\r\n'.join(headers)+'\r\n\r\n').encode('ascii'))
            response = bytearray()
            while len(response) < 8192 and not response.endswith(b'\r\n\r\n'):
                byte = desktop.recv(1)
                if not byte: break
                response.extend(byte)
            if not response.startswith(b'HTTP/1.1 101 ') or not response.endswith(b'\r\n\r\n'): self.reply(502, {'error':'Desktop handshake failed'}); return
            self.connection.sendall(response); self.close_connection = True
            try:
                while self.session() is session:
                    ready, _, _ = select.select([self.connection, desktop], [], [], 1)
                    if self.connection.pending() and self.connection not in ready: ready.append(self.connection)
                    for stream in ready:
                        data = stream.recv(65536)
                        if not data: return
                        (desktop if stream is self.connection else self.connection).sendall(data)
            except (OSError, ValueError): return
