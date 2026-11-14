#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Real HTTPS and Unix-FD tests with temporary roots; never touch host accounts."""
import http.client
import base64
import hashlib
import json
import os
from pathlib import Path
import socket
import ssl
import subprocess
import sys
import tempfile
import threading
import time
import unittest
from types import SimpleNamespace
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'src/remote'))
import provision
import broker
import rdp
import gateway


class ProvisionTests(unittest.TestCase):
    def test_one_time_verifier_validation_and_no_overwrite(self):
        with tempfile.TemporaryDirectory() as directory:
            boot = Path(directory)
            result = provision.prepare(boot, 'setup-password-123', 'test-pi')
            config = provision.validate(provision.read_json(boot/'fidan-setup.json'))
            self.assertEqual(result['hostname'], 'test-pi.local')
            self.assertNotIn('setup-password-123', (boot/'fidan-setup.json').read_text())
            self.assertTrue(provision.verify_password('setup-password-123', config))
            self.assertFalse(provision.verify_password('wrong', config))
            self.assertFalse(provision.verify_password([], config))
            with self.assertRaises(FileExistsError): provision.prepare(boot, 'another-password')
            config['hostname'] = '../etc/passwd'
            with self.assertRaises(ValueError): provision.validate(config)
            (boot/'link').symlink_to(boot/'fidan-setup.json')
            with self.assertRaises(OSError): provision.read_json(boot/'link')

    def test_password_byte_limit_and_hostname(self):
        with tempfile.TemporaryDirectory() as directory:
            for password, hostname in [('short','fidan'), ('ü'*40,'fidan'), ('valid-password','bad/name'), ('valid\npassword','fidan')]:
                with self.assertRaises(ValueError): provision.prepare(directory, password, hostname)

    def test_cleanup_waits_for_completion(self):
        with tempfile.TemporaryDirectory() as directory:
            paths = provision.Paths(directory)
            provision.write_private(paths.provision, {'test': True})
            provision.cleanup(paths)
            self.assertTrue(paths.provision.exists())
            paths.at('/var/lib/fidan/firstboot-complete').touch()
            provision.cleanup(paths)
            self.assertFalse(paths.provision.exists())

    def test_bootstrap_consumes_card_and_builds_private_ap_profile(self):
        with tempfile.TemporaryDirectory() as directory:
            paths=provision.Paths(directory);boot=paths.at('/boot/firmware');boot.mkdir(parents=True)
            provision.prepare(boot,'setup-password-123','workshop');calls=[]
            def run(args,**kwargs):
                calls.append(args)
                if args[0]=='openssl':
                    Path(args[args.index('-keyout')+1]).write_text('private test key')
                    Path(args[args.index('-out')+1]).write_text('public test certificate')
                return subprocess.CompletedProcess(args,0,stdout='wifi:disconnected\nethernet:unavailable\n')
            provision.bootstrap(paths,run)
            self.assertFalse((boot/'fidan-setup.json').exists())
            self.assertEqual(paths.provision.stat().st_mode&0o777,0o640)
            self.assertEqual(paths.at('/var/lib/fidan/tls/key.pem').stat().st_mode&0o777,0o640)
            profile=paths.at('/etc/NetworkManager/system-connections/fidan-setup.nmconnection')
            self.assertEqual(profile.stat().st_mode&0o777,0o600)
            self.assertIn('mode=ap',profile.read_text());self.assertNotIn('setup-password-123',profile.read_text())
            self.assertIn(['hostnamectl','set-hostname','workshop'],calls)
            self.assertIn(['nmcli','--wait','20','connection','up','fidan-setup'],calls)
            paths.at('/var/lib/fidan/firstboot-complete').touch();provision.bootstrap(paths,run)
            self.assertFalse(profile.exists());self.assertFalse(paths.provision.exists())

    def test_connected_ethernet_does_not_start_access_point(self):
        with tempfile.TemporaryDirectory() as directory:
            paths=provision.Paths(directory);boot=paths.at('/boot/firmware');boot.mkdir(parents=True)
            provision.prepare(boot,'setup-password-123');calls=[]
            def run(args,**kwargs):
                calls.append(args)
                if args[0]=='openssl':
                    Path(args[args.index('-keyout')+1]).touch();Path(args[args.index('-out')+1]).touch()
                return subprocess.CompletedProcess(args,0,stdout='ethernet:connected\n')
            provision.bootstrap(paths,run)
            self.assertFalse(paths.at('/etc/NetworkManager/system-connections/fidan-setup.nmconnection').exists())
            self.assertFalse(any('fidan-setup' in command for command in calls))


class BrokerTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(); self.addCleanup(self.temp.cleanup)
        self.paths = provision.Paths(self.temp.name)
        for marker in ('/var/lib/fidan/firstboot-complete', '/etc/fidan/remote-enabled'):
            path = self.paths.at(marker); path.parent.mkdir(parents=True, exist_ok=True); path.touch()

    def setup_credentials(self):
        self.paths.at('/var/lib/fidan/firstboot-complete').unlink()
        boot = self.paths.at('/boot'); boot.mkdir()
        provision.prepare(boot, 'setup-password-123')
        provision.write_private(self.paths.provision, provision.read_json(boot/'fidan-setup.json'))

    def test_setup_token_captures_only_setup_account_and_is_revoked_on_completion(self):
        self.setup_credentials(); captures = []
        instance = broker.Broker(lambda *_: self.fail('Setup must not use PAM'),
            lambda uid, **flags: captures.append((uid, flags)), paths=self.paths,
            setup_account=lambda: SimpleNamespace(pw_uid=987, pw_name='fidan-setup'))
        with self.assertRaises(PermissionError): instance.login('maker', 'setup-password-123')
        with self.assertRaises(PermissionError): instance.login(None, 'wrong', setup=True)
        token = instance.login('root', 'setup-password-123', setup=True)
        instance.connect(token)
        self.assertEqual(captures, [(987, {'setup': True})])
        self.paths.at('/var/lib/fidan/firstboot-complete').touch()
        with self.assertRaises(PermissionError): instance.connect(token)
        with self.assertRaises(PermissionError): instance.login(None, 'setup-password-123', setup=True)

    def test_setup_rejects_root_and_any_other_system_identity(self):
        self.setup_credentials()
        for uid, name in ((0, 'fidan-setup'), (999, 'another-account')):
            instance = broker.Broker(paths=self.paths, setup_account=lambda: SimpleNamespace(pw_uid=uid, pw_name=name))
            with self.assertRaises(PermissionError): instance.login(None, 'setup-password-123', setup=True)

    def test_setup_report_is_bounded_redacted_and_requires_live_setup_token(self):
        self.setup_credentials()
        instance = broker.Broker(paths=self.paths, setup_account=lambda: SimpleNamespace(pw_uid=987, pw_name='fidan-setup'))
        token = instance.login(None, 'setup-password-123', setup=True)
        calls = []
        def run(command, **kwargs):
            calls.append((command, kwargs))
            return subprocess.CompletedProcess(command, 0, stdout=b'No input devices found\npassword=hidden-value\n' + b'x'*40000)
        with patch.object(broker.subprocess, 'run', side_effect=run):
            report = instance.diagnostics(token)
            self.assertNotIn('hidden-value', json.dumps(report)); self.assertIn('[redacted]', report['journal'])
            self.assertLessEqual(len(report['journal']), 20000)
            self.assertNotIn('-f', calls[1][0]); self.assertIn('60', calls[1][0])
            self.assertTrue(all(options['timeout'] == 5 for _, options in calls))
            before = len(calls)
            with self.assertRaises(PermissionError): instance.diagnostics('wrong')
            self.paths.at('/var/lib/fidan/firstboot-complete').touch()
            with self.assertRaises(PermissionError): instance.diagnostics(token)
            self.assertEqual(len(calls), before)

    def test_report_ipc_accepts_bounded_reply_larger_than_desktop_control_limit(self):
        self.setup_credentials()
        instance = broker.Broker(paths=self.paths, setup_account=lambda: SimpleNamespace(pw_uid=987, pw_name='fidan-setup'))
        token = instance.login(None, 'setup-password-123', setup=True)
        with patch.object(broker, 'setup_diagnostics', return_value={'journal': 'test-line\n'*1500}):
            with broker.BrokerServer(Path(self.temp.name)/'report-broker', os.getuid(), instance) as server:
                thread = threading.Thread(target=server.serve_forever, daemon=True); thread.start()
                try:
                    result = broker.request({'command':'diagnostics','token':token}, Path(self.temp.name)/'report-broker')
                    self.assertEqual(len(result['report']['journal']), 15000)
                finally: server.shutdown(); thread.join()

    def test_connection_report_records_capture_failure_without_credentials(self):
        self.setup_credentials()
        def capture(*args, **kwargs): raise RuntimeError('Untrusted desktop socket')
        instance = broker.Broker(capture=capture,paths=self.paths,setup_account=lambda: SimpleNamespace(pw_uid=987,pw_name='fidan-setup'))
        token = instance.login(None,'setup-password-123',setup=True)
        with self.assertRaises(RuntimeError): instance.connect(token)
        with patch.object(broker,'setup_diagnostics',return_value={}):
            report=instance.diagnostics(token)
        self.assertEqual(report['lastCaptureFailure']['message'],'Untrusted desktop socket')
        self.assertNotIn(token,json.dumps(report)); self.assertNotIn('setup-password-123',json.dumps(report))

    def test_disabled_desktop_revokes_existing_token(self):
        instance = broker.Broker(lambda *_: 1000, lambda *_: self.fail('No capture after revocation'), paths=self.paths)
        token = instance.login('maker', 'password')
        self.paths.at('/etc/fidan/remote-enabled').unlink()
        with self.assertRaises(PermissionError): instance.connect(token)

    def test_setup_capture_starts_shared_native_session(self):
        account = SimpleNamespace(pw_name='fidan-setup', pw_uid=987)
        with patch.object(broker.pwd, 'getpwuid', return_value=account), patch.object(broker, 'find_wayland', return_value=None), \
             patch.object(broker.time, 'sleep'), patch.object(broker.subprocess, 'run') as run:
            with self.assertRaises(RuntimeError): broker.capture_socket(987, setup=True)
            self.assertEqual(run.call_args.args[0], ['systemctl', 'start', 'fidan-firstboot-session.service'])

    def test_output_helper_is_created_by_manager_without_switching_broker_credentials(self):
        account=SimpleNamespace(pw_name='fidan-setup',pw_uid=987,pw_gid=987,pw_dir='/var/lib/fidan-setup')
        with patch.object(broker.pwd,'getpwuid',return_value=account), patch.object(broker,'find_wayland',return_value='wayland-0'), \
             patch.object(broker.Path,'lstat',side_effect=FileNotFoundError), patch.object(broker.time,'sleep'), \
             patch.object(broker.subprocess,'run') as run:
            with self.assertRaises(RuntimeError): broker.capture_socket(987,setup=True)
            command=run.call_args_list[0].args[0]
            self.assertEqual(command[0],'systemd-run'); self.assertIn('--wait',command)
            self.assertIn('--uid=fidan-setup',command); self.assertIn('--unit=fidan-remote-output-987',command)
            self.assertEqual(command[-2:],['/usr/bin/fidan-remote-session','prepare-output'])
            self.assertFalse({'user','group','extra_groups'} & run.call_args_list[0].kwargs.keys())

    def test_authentication_expiry_and_logout(self):
        calls=[]
        instance=broker.Broker(lambda user,password: 1000 if (user,password)==('maker','secret') else None, calls.append, paths=self.paths)
        with self.assertRaises(PermissionError): instance.login('maker','wrong')
        token=instance.login('maker','secret');instance.connect(token)
        self.assertEqual(calls,[1000])
        instance.logout(token)
        with self.assertRaises(PermissionError): instance.connect(token)
        with self.assertRaises(ValueError): instance.connect({})
        token=instance.login('maker','secret');instance.sessions[token]=(1000,0,'desktop')
        with self.assertRaises(PermissionError): instance.connect(token)
        for _ in range(8):
            try: instance.login('maker','wrong')
            except PermissionError: pass
        with self.assertRaises(PermissionError): instance.login('maker','secret')

    def test_unix_peer_authentication_and_fd_transfer(self):
        with tempfile.TemporaryDirectory() as directory:
            left,right=socket.socketpair()
            instance=broker.Broker(lambda *_:1000, lambda uid:left.dup(), paths=self.paths)
            with broker.BrokerServer(Path(directory)/'broker',os.getuid(),instance) as server:
                thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
                try:
                    call=lambda data,**kwargs:broker.request(data,Path(directory)/'broker',**kwargs)
                    token=call({'command':'login','username':'maker','password':'secret'})['token']
                    with call({'command':'connect','token':token},receive_fd=True) as received:
                        received.sendall(b'private-desktop');self.assertEqual(right.recv(64),b'private-desktop')
                    call({'command':'logout','token':token})
                    with self.assertRaises(PermissionError):call({'command':'connect','token':token},receive_fd=True)
                    server.gateway_uid=os.getuid()+1
                    with self.assertRaises((OSError,ConnectionError)):call({'command':'login'})
                finally:server.shutdown();thread.join();left.close();right.close()

    def test_setup_login_over_ipc_delivers_only_setup_capture(self):
        self.setup_credentials(); left, right = socket.socketpair(); captures = []
        def capture(uid, **flags): captures.append((uid, flags)); return left.dup()
        instance = broker.Broker(capture=capture, paths=self.paths,
            setup_account=lambda: SimpleNamespace(pw_uid=987, pw_name='fidan-setup'))
        with broker.BrokerServer(Path(self.temp.name)/'broker', os.getuid(), instance) as server:
            thread = threading.Thread(target=server.serve_forever, daemon=True); thread.start()
            try:
                token = broker.request({'command':'setup-login', 'password':'setup-password-123'}, Path(self.temp.name)/'broker')['token']
                with broker.request({'command':'connect', 'token':token}, Path(self.temp.name)/'broker', receive_fd=True) as desktop:
                    desktop.sendall(b'native-setup'); self.assertEqual(right.recv(64), b'native-setup')
                self.assertEqual(captures, [(987, {'setup': True})])
            finally: server.shutdown(); thread.join(); left.close(); right.close()


class RdpTests(unittest.TestCase):
    def test_optional_setup_stops_stock_listener_and_limits_sessions(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory); config = root/'etc/xrdp'; config.mkdir(parents=True)
            (config/'xrdp.ini').write_text('[Globals]\nport=3389\n[Xorg]\nparam=Xorg\nparam=-nolisten\n')
            (config/'sesman.ini').write_text('[Globals]\n[Security]\n[Sessions]\n[Xorg]\nparam=Xorg\n')
            calls = []; written = {}
            def run(args, **kwargs):
                calls.append(args)
                if args[0] == 'openssl':
                    Path(args[args.index('-keyout')+1]).touch(); Path(args[args.index('-out')+1]).touch()
                return subprocess.CompletedProcess(args, 0)
            account = SimpleNamespace(pw_uid=1000, pw_shell='/bin/bash')
            with patch.object(rdp.pwd, 'getpwnam', return_value=account), patch.object(rdp.grp, 'getgrnam', return_value=SimpleNamespace(gr_gid=os.getgid())), \
                 patch.object(rdp.shutil, 'which', return_value='/usr/bin/tool'), patch.object(rdp.os, 'chown'), \
                 patch.object(rdp, 'write_config', side_effect=lambda path,text: written.update({path.name:text})):
                rdp.enable('maker', root, run)
            self.assertEqual(calls[0], ['systemctl','stop','xrdp.service','xrdp-sesman.service'])
            self.assertIn('port=tcp://127.0.0.1:3389', written['xrdp.ini']); self.assertIn('security_layer=tls', written['xrdp.ini'])
            self.assertIn('MaxSessions=1', written['sesman.ini']); self.assertIn('AllowRootLogin=false', written['sesman.ini'])
            self.assertIn('AlwaysGroupCheck=true', written['sesman.ini']); self.assertIn('KillDisconnected=true', written['sesman.ini'])
            self.assertEqual(calls[-1], ['systemctl','enable','--now','xrdp-sesman.service','xrdp.service'])
    def test_replaces_duplicate_listener_settings_without_losing_xorg_arguments(self):
        original = '[Globals]\nport=3389\nPORT=tcp://0.0.0.0:3390\nsecurity_layer=rdp\n[Xorg]\nparam=Xorg\nparam=-nolisten\nparam=tcp\n'
        result = rdp.ini_values(original, 'Globals', {'port':'tcp://127.0.0.1:3389', 'security_layer':'tls'})
        self.assertNotIn('0.0.0.0', result); self.assertEqual(result.count('port='), 1)
        self.assertIn('security_layer=tls', result); self.assertIn('param=Xorg\nparam=-nolisten\nparam=tcp', result)
        self.assertEqual(rdp.ini_values(result, 'Globals', {'port':'tcp://127.0.0.1:3389', 'security_layer':'tls'}), result)
        with self.assertRaises(ValueError): rdp.ini_values('[Globals]\n[Globals]\n', 'Globals', {'port':'3389'})
    def test_invalid_or_system_accounts_never_start_services(self):
        for username in ('root', '--option', 'user\ninjection'):
            with patch.object(rdp.subprocess, 'run') as run:
                with self.assertRaises(ValueError): rdp.enable(username)
                run.assert_not_called()


class GatewayTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.certdir=tempfile.TemporaryDirectory();root=Path(cls.certdir.name)
        subprocess.run(['openssl','req','-x509','-newkey','rsa:2048','-nodes','-days','1','-subj','/CN=localhost',
                        '-keyout',str(root/'key'),'-out',str(root/'cert')],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
        cls.tls=ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER);cls.tls.load_cert_chain(root/'cert',root/'key')
    @classmethod
    def tearDownClass(cls):cls.certdir.cleanup()
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory();self.paths=provision.Paths(self.tmp.name)
        self.boot=self.paths.at('/boot');self.boot.mkdir();provision.prepare(self.boot,'setup-password-123')
        provision.write_private(self.paths.provision,provision.read_json(self.boot/'fidan-setup.json'))
        self.calls=[]
        def ipc(data, **kwargs):
            self.calls.append(data)
            if data['command']=='setup-login' and not provision.verify_password(data.get('password'), provision.read_json(self.paths.provision)):
                raise PermissionError('Sign-in failed')
            return {'token':'broker-token'}
        self.server=gateway.Gateway(('127.0.0.1',0),self.tls,self.paths,ROOT/'data/remote',ROOT/'data/remote',ipc)
        self.thread=threading.Thread(target=self.server.serve_forever,daemon=True);self.thread.start()
        self.cookie='';self.csrf=''
    def tearDown(self):self.server.shutdown();self.thread.join();self.server.server_close();self.tmp.cleanup()
    def request(self,path,body=None,headers=None):
        connection=http.client.HTTPSConnection('127.0.0.1',self.server.server_port,context=ssl._create_unverified_context(),timeout=5)
        values={'Origin':f'https://127.0.0.1:{self.server.server_port}','Cookie':self.cookie,'X-CSRF-Token':self.csrf}
        if body is not None:values['Content-Type']='application/json'
        values.update(headers or {})
        connection.request('POST' if body is not None else 'GET',path,json.dumps(body) if body is not None else None,values)
        response=connection.getresponse();data=response.read();result=(response.status,data,dict(response.getheaders()));connection.close();return result
    def login(self):
        status,data,headers=self.request('/api/login',{'password':'setup-password-123'})
        self.assertEqual(status,200);self.cookie=headers['Set-Cookie'].split(';')[0];self.csrf=json.loads(data)['csrf']
        self.assertIn('Secure; HttpOnly; SameSite=Strict',headers['Set-Cookie'])
    def test_https_login_csrf_resume_and_logout(self):
        self.assertEqual(self.request('/api/state')[0],401)
        self.assertEqual(self.request('/api/login',{'password':'bad'})[0],401)
        self.login()
        status,data,headers=self.request('/api/state');self.assertEqual(status,200);self.assertEqual(json.loads(data)['role'],'setup')
        self.assertIn("frame-ancestors 'none'",headers['Content-Security-Policy'])
        body={'operation':'skip-network','arguments':[]}
        self.assertEqual(self.request('/api/setup',body,{'X-CSRF-Token':'wrong'})[0],403)
        self.assertEqual(self.request('/api/setup',body)[0],403)
        self.assertEqual(self.calls[-1]['command'],'setup-login')
        self.assertEqual(self.request('/api/logout',{})[0],200)
        self.assertEqual(self.request('/api/state')[0],401)
    def test_host_origin_and_authenticated_static_boundaries(self):
        self.assertEqual(self.request('/',headers={'Host':'attacker.example'})[0],403)
        self.assertEqual(self.request('/',headers={'Host':'fidan.local#attacker'})[0],403)
        self.assertEqual(self.request('/',headers={'Host':'fidan.local?attacker'})[0],403)
        self.assertEqual(self.request('/api/login',{}, {'Origin':'https://attacker.example'})[0],403)
        self.assertEqual(self.request('/novnc/ui.js')[0],401)
        self.login()
        # Setup credentials open the native setup desktop, never a user desktop.
        self.assertEqual(self.request('/novnc/ui.js')[0],200)
        self.assertEqual(self.request('/desktop-socket')[0],400)
        self.assertEqual(self.request('/api/setup',{}, {'Content-Type':'text/plain'})[0],400)

    def test_portal_wallpaper_uses_only_the_fixed_asset(self):
        wallpaper=Path(self.tmp.name)/'wallpaper.png';wallpaper.write_bytes(b'png-fixture')
        self.server.wallpaper=wallpaper
        status,data,_=self.request('/wallpaper.png');self.assertEqual(status,200);self.assertEqual(data,b'png-fixture')
        self.assertEqual(self.request('/wallpaper.png/../provision.json')[0],404)

    def test_connection_report_requires_setup_login(self):
        self.assertEqual(self.request('/api/diagnostics')[0],403)
        self.login()
        calls = []
        def ipc(data, **kwargs):
            calls.append(data); return {'report': {'schema':1, 'journal':'safe bounded report'}}
        self.server.ipc = ipc
        status, body, _ = self.request('/api/diagnostics')
        self.assertEqual(status,200); self.assertEqual(json.loads(body)['schema'],1)
        self.assertEqual(calls[-1]['command'],'diagnostics')
        self.paths.at('/var/lib/fidan/firstboot-complete').touch()
        self.assertEqual(self.request('/api/diagnostics')[0],403)
    def test_completion_revokes_setup_and_remote_is_opt_in(self):
        self.login();self.paths.at('/var/lib/fidan/firstboot-complete').touch()
        self.assertEqual(self.request('/api/state')[0],403)
        flag=self.paths.at('/etc/fidan/remote-enabled');flag.parent.mkdir(parents=True);flag.touch()
        self.assertEqual(self.request('/api/state')[0],401)
        status,data,headers=self.request('/api/login',{'username':'maker','password':'local-password'})
        self.assertEqual(status,200);self.cookie=headers['Set-Cookie'].split(';')[0];self.csrf=json.loads(data)['csrf']
        self.assertEqual(self.request('/novnc/ui.js')[0],200)
        self.assertEqual(self.request('/novnc/%2e%2e/%2e%2e/README.md')[0],404)
        self.assertEqual(self.request('/api/setup',{'operation':'complete'})[0],403)
        flag.unlink();self.assertEqual(self.request('/api/state')[0],403)
    def test_rate_limit_and_expiry(self):
        self.login()
        for value in self.server.sessions.items.values():value['expires']=0
        self.assertEqual(self.request('/api/state')[0],401)
        for _ in range(4):self.assertEqual(self.request('/api/login',{'password':'bad'})[0],401)
        self.assertEqual(self.request('/api/login',{'password':'setup-password-123'})[0],429)
    def test_web_setup_mutations_are_removed(self):
        self.login()
        for operation in ('account', 'privacy', 'complete', 'not-a-method'):
            self.assertEqual(self.request('/api/setup', {'operation':operation, 'arguments':[]})[0],403)

    def test_authenticated_setup_websocket_tunnels_frames_and_revokes_on_completion(self):
        self.login(); left, right = socket.socketpair(); received = []
        right.settimeout(3)
        def peer():
            headers = bytearray()
            while not headers.endswith(b'\r\n\r\n'): headers.extend(right.recv(1))
            received.append(bytes(headers))
            key = b'dGVzdC13ZWJzb2NrZXQta2V5'
            accept = base64.b64encode(hashlib.sha1(key+b'258EAFA5-E914-47DA-95CA-C5AB0DC85B11').digest())
            right.sendall(b'HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: '+accept+b'\r\n\r\n'+b'\x82\x06native')
        def ipc(data, **flags):
            self.assertEqual(data, {'command':'connect', 'token':'broker-token'}); self.assertTrue(flags['receive_fd']); return left
        self.server.ipc = ipc
        thread = threading.Thread(target=peer, daemon=True); thread.start()
        context=ssl._create_unverified_context()
        with context.wrap_socket(socket.create_connection(('127.0.0.1', self.server.server_port)), server_hostname='localhost') as browser:
            browser.settimeout(3)
            host=f'127.0.0.1:{self.server.server_port}'
            browser.sendall((f'GET /desktop-socket HTTP/1.1\r\nHost: {host}\r\nOrigin: https://{host}\r\nCookie: {self.cookie}\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Key: dGVzdC13ZWJzb2NrZXQta2V5\r\n\r\n').encode())
            headers=bytearray()
            while not headers.endswith(b'\r\n\r\n'): headers.extend(browser.recv(1))
            self.assertTrue(headers.startswith(b'HTTP/1.1 101 '))
            payload=bytearray()
            while len(payload)<8: payload.extend(browser.recv(8-len(payload)))
            self.assertEqual(payload, b'\x82\x06native')
            self.paths.at('/var/lib/fidan/firstboot-complete').touch()
            self.assertEqual(browser.recv(1), b'')
        thread.join(); right.close(); self.assertTrue(received)




if __name__=='__main__':unittest.main()
