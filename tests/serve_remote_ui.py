#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Local browser UI fixture. No PAM, accounts, network setup or device writes.

Run with python3 tests/serve_remote_ui.py, then open http://localhost:8765/checks.
The RFB fixture tests the viewport and connection lifecycle, not VNC transport.
"""
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
from pathlib import Path
import time
from urllib.parse import urlsplit
import argparse
import select
import socket

ROOT = Path(__file__).resolve().parents[1]
RFB = """
export default class RFB extends EventTarget {
    constructor(screen) {
        super(); this.screen=screen;
        const surface=document.createElement('div');
        surface.textContent='PiOS · Desktop connection fixture';
        surface.style.cssText='width:100%;height:100%;display:grid;place-items:center;color:#cbd5ff;background:#232b43';
        screen.append(surface);
        screen.dataset.connections=String(Number(screen.dataset.connections||0)+1);
        this.timer=setTimeout(()=>{
            if(Number(screen.dataset.failConnections||0)>0){screen.dataset.failConnections=String(Number(screen.dataset.failConnections)-1);this.disconnect();}
            else this.dispatchEvent(new Event('connect'));
        },100);
        this.drop=()=>this.disconnect();screen.addEventListener('test-disconnect',this.drop);
    }
    disconnect(){clearTimeout(this.timer);this.screen.removeEventListener('test-disconnect',this.drop);this.dispatchEvent(new Event('disconnect'));}
    focus(){}
    clipboardPasteFrom(value){this.screen.dataset.clipboard=value;}
}
"""


class Handler(BaseHTTPRequestHandler):
    role = ''
    mode = 'setup'
    state = {'step': 1}
    native_socket = None
    keepalives = 0
    renewal = True
    login_requests = 0
    cooldown = False
    username = ''

    def log_message(self, *args):
        pass

    def reply(self, value, status=200, kind='application/json'):
        data = json.dumps(value).encode() if kind == 'application/json' else value.encode()
        self.send_response(status)
        self.send_header('Content-Type', kind + '; charset=utf-8')
        self.send_header('Cache-Control', 'no-store')
        self.send_header('Content-Length', str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def do_GET(self):
        path = urlsplit(self.path).path
        if path == '/api/mode':
            time.sleep(.3)  # Keep the startup state observable in the fixture.
            return self.reply({'mode': Handler.mode})
        if path == '/api/state':
            if not Handler.role:
                return self.reply({'error': 'Sign in again'}, 401)
            return self.reply({'role': Handler.role, 'csrf': 'test-only', 'mode': Handler.mode, 'sessionRenewal':Handler.renewal})
        if path == '/__test/state':return self.reply({'keepalives':Handler.keepalives,'loginRequests':Handler.login_requests,'username':Handler.username})
        if path == '/desktop-socket' and self.native_socket and Handler.role:
            with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as desktop:
                desktop.connect(str(self.native_socket))
                desktop.sendall(('GET / HTTP/1.1\r\n'+''.join(key+': '+value+'\r\n' for key,value in self.headers.items())+'\r\n').encode())
                self.close_connection = True
                while True:
                    ready, _, _ = select.select([self.connection, desktop], [], [], 1)
                    for stream in ready:
                        data = stream.recv(65536)
                        if not data: return
                        (desktop if stream is self.connection else self.connection).sendall(data)
        if path == '/novnc/core/rfb.js' and not self.native_socket:
            return self.reply(RFB, kind='text/javascript')
        if path.startswith('/novnc/') and self.native_socket:
            root = Path('/usr/share/novnc').resolve(); candidate=(root/path[len('/novnc/'):]).resolve()
            if candidate.is_relative_to(root) and candidate.is_file():
                return self.reply(candidate.read_text(), kind='text/javascript')
        files = {'/': ('data/remote/index.html', 'text/html'),
                 '/style.css': ('data/remote/style.css', 'text/css'),
                 '/ui.js': ('data/remote/ui.js', 'text/javascript'),
                 '/checks.js': ('tests/remote_ui_checks.js', 'text/javascript')}
        if path == '/checks':
            return self.reply('<!doctype html><html><head><title>PiOS browser checks</title></head>'
                              '<body><h1>PiOS browser checks</h1><pre id="results">Running…</pre>'
                              '<iframe title="UI under test" style="border:0"></iframe>'
                              '<script type="module" src="/checks.js"></script></body></html>', kind='text/html')
        if path in files:
            file, kind = files[path]
            content=(ROOT/file).read_text(encoding='utf-8')
            if path=='/' and not self.native_socket:
                # Test-only accelerated timers; the shipped UI retains real delays.
                timers="<script>const timeout=window.setTimeout.bind(window),interval=window.setInterval.bind(window);window.setTimeout=(fn,ms,...args)=>timeout(fn,[1000,2000,4000,8000,12000].includes(ms)?150:ms,...args);window.setInterval=(fn,ms,...args)=>interval(fn,ms===60000?500:ms,...args);</script>"
                content=content.replace('<head>','<head>'+timers)
            return self.reply(content, kind=kind)
        if path == '/wallpaper.png':
            data=(ROOT/'artwork/wallpapers/fidan-mountain-dawn.png').read_bytes()
            self.send_response(200);self.send_header('Content-Type','image/png');self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data);return
        self.reply({'error': 'Not found'}, 404)

    def do_POST(self):
        body = json.loads(self.rfile.read(int(self.headers['Content-Length'])))
        path = urlsplit(self.path).path
        if path == '/__test/reset':
            Handler.mode = body.get('mode', 'setup')
            Handler.role = body.get('role', '')
            Handler.state = body.get('state', {'step': 1})
            Handler.keepalives=0;Handler.renewal=body.get('renewal',True)
            Handler.login_requests=0;Handler.cooldown=body.get('cooldown',False);Handler.username=''
            return self.reply({'ok': True})
        if path == '/api/login':
            Handler.login_requests+=1;Handler.username=body.get('username','')
            if Handler.cooldown:
                Handler.cooldown=False
                return self.reply({'error':'Wait a minute before trying again','retryAfter':2},429)
            time.sleep(.15)
            if body.get('password') != 'fixture-password':
                return self.reply({'error': 'Sign-in failed'}, 401)
            Handler.role = Handler.mode
            return self.reply({'role': Handler.role, 'csrf': 'test-only'})
        if path == '/api/logout':
            Handler.role = ''
            return self.reply({'ok': True})
        if path == '/api/keepalive':
            if not Handler.role or self.headers.get('X-CSRF-Token')!='test-only':return self.reply({'error':'Sign in again'},403)
            Handler.keepalives+=1;return self.reply({'ok':True})
        self.reply({'error': 'Not found'}, 404)


if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native-socket', type=Path, help='Use real noVNC and a live WayVNC Unix WebSocket (Linux only)')
    parser.add_argument('--port', type=int, default=8765)
    args=parser.parse_args(); Handler.native_socket=args.native_socket
    print(f'Local test fixture only: http://localhost:{args.port}/', flush=True)
    ThreadingHTTPServer(('127.0.0.1', args.port), Handler).serve_forever()
