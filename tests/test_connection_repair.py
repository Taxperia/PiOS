#!/usr/bin/python3
import importlib.machinery
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


def module(name, path):
    loader = importlib.machinery.SourceFileLoader(name, str(path)); spec = importlib.util.spec_from_loader(name, loader)
    value = importlib.util.module_from_spec(spec); loader.exec_module(value); return value


prepare = module('connection_prepare', ROOT/'scripts/fidan-repair-connection')
repair = module('connection_apply', ROOT/'data/repair/connection-report/apply')


class ConnectionRepairTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(); self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)/'root'; self.boot = Path(self.temp.name)/'boot'; self.boot.mkdir()
        self.command = b'console=tty1 root=/dev/disk/by-slot/system rootwait\n'
        for name in ('config.txt','kernel_2712.img','initramfs_2712'): (self.boot/name).write_bytes(b'fixture')
        (self.boot/'cmdline.txt').write_bytes(self.command)
        for name, target in repair.FILES.items():
            path = self.root/target.lstrip('/'); path.parent.mkdir(parents=True,exist_ok=True); path.write_bytes(('old-'+name).encode())
        marker = self.root/'etc/fidan/pi-image'; marker.parent.mkdir(parents=True); marker.touch()
        self.state = self.root/'var/lib/fidan/setup-state.json'; self.state.parent.mkdir(parents=True)
        self.state.write_text('private-saved-state')

    def stage(self):
        manifest = prepare.prepare(self.boot)
        for name, target in repair.FILES.items(): manifest['files'][name]['baseSha256'] = repair.sha((self.root/target.lstrip('/')).read_bytes())
        (self.boot/'fidan-connection-repair/manifest.json').write_text(json.dumps(manifest))

    def test_install_preserves_progress_and_original_boot(self):
        before = self.state.read_bytes(); self.stage()
        self.assertIn(b'systemd.unit=kernel-command-line.target',(self.boot/'cmdline.txt').read_bytes())
        self.assertTrue(repair.apply(self.root,self.boot)['ok'])
        self.assertEqual(self.state.read_bytes(),before); self.assertEqual((self.boot/'cmdline.txt').read_bytes(),self.command)
        for name, target in repair.FILES.items(): self.assertEqual((self.root/target.lstrip('/')).read_bytes(),(self.boot/'fidan-connection-repair'/name).read_bytes())

    def test_bad_payload_is_rejected_before_any_target_write(self):
        self.stage(); before = {name:(self.root/path.lstrip('/')).read_bytes() for name,path in repair.FILES.items()}
        (self.boot/'fidan-connection-repair/broker.py').write_text('changed')
        with self.assertRaises(ValueError): repair.apply(self.root,self.boot)
        for name,path in repair.FILES.items(): self.assertEqual((self.root/path.lstrip('/')).read_bytes(),before[name])
        self.assertEqual((self.boot/'cmdline.txt').read_bytes(),self.command)

    def test_wrong_volume_and_unfinished_repair_are_not_overwritten(self):
        (self.boot/'cmdline.txt').write_text('root=/dev/other\n')
        with self.assertRaises(ValueError): prepare.prepare(self.boot)
        (self.boot/'cmdline.txt').write_bytes(self.command); prepare.prepare(self.boot)
        before = (self.boot/'fidan-connection-repair/cmdline.original').read_bytes()
        with self.assertRaises(ValueError): prepare.prepare(self.boot)
        self.assertEqual((self.boot/'fidan-connection-repair/cmdline.original').read_bytes(),before)

    def test_capture_followup_keeps_the_first_repair_and_progress(self):
        prepare.prepare(self.boot)
        saved=(self.boot/'fidan-connection-repair/manifest.json').read_bytes()
        (self.boot/'cmdline.txt').write_bytes(self.command)
        manifest=prepare.prepare(self.boot,capture_fix=True)
        for name,path in repair.FILES.items(): manifest['files'][name]['baseSha256']=repair.sha((self.root/path.lstrip('/')).read_bytes())
        (self.boot/'fidan-capture-repair/manifest.json').write_text(json.dumps(manifest))
        before=self.state.read_bytes()
        self.assertTrue(repair.apply(self.root,self.boot,'fidan-capture-repair')['ok'])
        self.assertEqual((self.boot/'fidan-connection-repair/manifest.json').read_bytes(),saved)
        self.assertEqual(self.state.read_bytes(),before)
        self.assertEqual((self.boot/'cmdline.txt').read_bytes(),self.command)


if __name__ == '__main__': unittest.main()
