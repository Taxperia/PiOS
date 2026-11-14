#!/usr/bin/python3
# SPDX-License-Identifier: GPL-3.0-or-later
import importlib.machinery
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


def load(name, path):
    loader = importlib.machinery.SourceFileLoader(name, str(path))
    spec = importlib.util.spec_from_loader(name, loader)
    module = importlib.util.module_from_spec(spec)
    loader.exec_module(module)
    return module


prepare = load('prepare_repair', ROOT / 'scripts/fidan-repair-setup')
repair = load('apply_repair', ROOT / 'data/repair/setup-web/apply')


class RepairTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.base = Path(self.temp.name)
        # Exercise the legacy repair with an independent legacy payload. Current
        # native desktop sources intentionally cannot be deployed as a web hotfix.
        self.source = self.base / 'source'
        for name, pair in prepare.BASE.items():
            source = self.source / pair[0]; source.parent.mkdir(parents=True, exist_ok=True)
            source.write_bytes(b'# legacy gateway fixture\n' if name.endswith('.py') else b'legacy UI fixture\n')
        program = self.source / 'data/repair/setup-web/apply'; program.parent.mkdir(parents=True, exist_ok=True)
        program.write_bytes((ROOT / 'data/repair/setup-web/apply').read_bytes())
        self.boot = self.base / 'boot'; self.boot.mkdir()
        for name in ('config.txt', 'kernel_2712.img', 'initramfs_2712'):
            (self.boot / name).write_bytes(b'fixture')
        self.cmdline = b'console=tty1 root=/dev/disk/by-slot/system rootwait\n'
        (self.boot / 'cmdline.txt').write_bytes(self.cmdline)
        self.root = self.base / 'root'
        for name, path in repair.FILES.items():
            target = self.root / path.lstrip('/')
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(('previous-' + name).encode())
        marker = self.root / 'etc/fidan/pi-image'
        marker.parent.mkdir(parents=True); marker.touch()
        self.state = self.root / 'var/lib/fidan/setup-state.json'
        self.state.parent.mkdir(parents=True)
        self.state.write_text('{"username":"maker","accountReady":true}')
        self.accounts = self.root / 'etc/passwd'
        self.accounts.write_text('maker:x:1000:1000:Maker:/home/maker:/bin/bash\n')

    def stage(self):
        manifest = prepare.prepare(self.boot, self.source)
        for name, path in repair.FILES.items():
            manifest['files'][name]['baseSha256'] = repair.digest((self.root / path.lstrip('/')).read_bytes())
        (self.boot / 'fidan-repair/manifest.json').write_text(json.dumps(manifest))

    def test_repair_preserves_state_and_accounts_and_restores_normal_boot(self):
        state, accounts = self.state.read_bytes(), self.accounts.read_bytes()
        self.stage()
        self.assertIn(b'systemd.run=', (self.boot / 'cmdline.txt').read_bytes())
        self.assertIn(b'systemd.unit=kernel-command-line.target', (self.boot / 'cmdline.txt').read_bytes())
        result = repair.apply(self.root, self.boot)
        self.assertTrue(result['ok'])
        self.assertEqual(self.state.read_bytes(), state)
        self.assertEqual(self.accounts.read_bytes(), accounts)
        self.assertEqual((self.boot / 'cmdline.txt').read_bytes(), self.cmdline)
        for name, path in repair.FILES.items():
            self.assertEqual((self.root / path.lstrip('/')).read_bytes(), (self.boot / 'fidan-repair' / name).read_bytes())
            self.assertEqual((self.root / 'var/lib/fidan/repairs' / repair.REPAIR / name).read_bytes(), ('previous-' + name).encode())
        self.assertTrue(repair.apply(self.root, self.boot)['ok'])

    def test_bad_payload_restores_boot_and_does_not_install_any_file(self):
        before = {path: (self.root / path.lstrip('/')).read_bytes() for path in repair.FILES.values()}
        self.stage()
        (self.boot / 'fidan-repair/ui.js').write_text('tampered')
        with self.assertRaises(ValueError):
            repair.apply(self.root, self.boot)
        self.assertEqual((self.boot / 'cmdline.txt').read_bytes(), self.cmdline)
        for path, content in before.items():
            self.assertEqual((self.root / path.lstrip('/')).read_bytes(), content)

    def test_unknown_installed_version_is_not_overwritten(self):
        self.stage()
        target = self.root / repair.FILES['gateway.py'].lstrip('/')
        target.write_text('custom installation')
        with self.assertRaises(ValueError):
            repair.apply(self.root, self.boot)
        self.assertEqual(target.read_text(), 'custom installation')
        self.assertEqual((self.boot / 'cmdline.txt').read_bytes(), self.cmdline)

    def test_wrong_volume_and_existing_boot_override_are_rejected(self):
        (self.boot / 'kernel_2712.img').unlink()
        with self.assertRaises(ValueError):
            prepare.prepare(self.boot)
        self.assertFalse((self.boot / 'fidan-repair').exists())
        (self.boot / 'kernel_2712.img').touch()
        (self.boot / 'cmdline.txt').write_bytes(self.cmdline.strip() + b' init=/bin/sh\n')
        with self.assertRaises(ValueError):
            prepare.prepare(self.boot)

    def test_native_desktop_sources_cannot_be_installed_as_a_web_only_repair(self):
        before = (self.boot / 'cmdline.txt').read_bytes()
        with self.assertRaisesRegex(ValueError, 'matching native desktop packages'):
            prepare.prepare(self.boot)
        self.assertEqual((self.boot / 'cmdline.txt').read_bytes(), before)
        self.assertFalse((self.boot / 'fidan-repair').exists())

    def test_existing_repair_is_not_replaced(self):
        self.stage()
        before = (self.boot / 'cmdline.txt').read_bytes()
        with self.assertRaises(ValueError):
            prepare.prepare(self.boot, self.source)
        self.assertEqual((self.boot / 'cmdline.txt').read_bytes(), before)

    def test_resume_adds_explicit_target_without_replacing_backup(self):
        prepare.prepare(self.boot, self.source)
        original = (self.boot / 'fidan-repair/cmdline.original').read_bytes()
        old = self.cmdline.strip() + b' ' + prepare.RUN.rsplit(' systemd.unit=', 1)[0].encode() + b'\n'
        (self.boot / 'cmdline.txt').write_bytes(old)
        prepare.resume(self.boot, self.source)
        self.assertIn(b'systemd.unit=kernel-command-line.target', (self.boot / 'cmdline.txt').read_bytes())
        self.assertEqual((self.boot / 'fidan-repair/cmdline.original').read_bytes(), original)

    def test_resume_rejects_tampered_payload_and_unrelated_boot(self):
        prepare.prepare(self.boot, self.source)
        before = (self.boot / 'cmdline.txt').read_bytes()
        (self.boot / 'fidan-repair/ui.js').write_text('changed')
        with self.assertRaises(ValueError):
            prepare.resume(self.boot, self.source)
        self.assertEqual((self.boot / 'cmdline.txt').read_bytes(), before)
        (self.boot / 'cmdline.txt').write_text('root=/dev/other\n')
        with self.assertRaises(ValueError):
            prepare.resume(self.boot, self.source)
        self.assertEqual((self.boot / 'cmdline.txt').read_text(), 'root=/dev/other\n')


if __name__ == '__main__':
    unittest.main()
