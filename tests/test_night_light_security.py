# SPDX-License-Identifier: GPL-3.0-or-later
"""Test hostile Night Light runtime entries without changing display gamma."""
import contextlib
import hashlib
import importlib.machinery
import importlib.util
import io
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

ROOT=Path(__file__).resolve().parents[1]
loader=importlib.machinery.SourceFileLoader('night_light',str(ROOT/'scripts/fidan-night-light'))
spec=importlib.util.spec_from_loader(loader.name,loader)
light=importlib.util.module_from_spec(spec)
loader.exec_module(light)

class NightLightSecurityTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup)
        self.root=Path(self.temp.name)
        self.runtime=self.root/'runtime';self.runtime.mkdir(mode=0o700)
        self.private=self.runtime/'fidan-night-light'
        self.config=self.root/'config/night-light.json'
        self.victim=self.root/'important.conf';self.victim.write_text('keep this')
        self.settings=patch.multiple(light,runtime_root=self.runtime,runtime=self.private,config=self.config)
        self.settings.start();self.addCleanup(self.settings.stop)

    def run_light(self,*args):
        with patch.object(light.os,'geteuid',return_value=1000),patch.object(light.sys,'argv',['fidan-night-light',*args]),contextlib.redirect_stdout(io.StringIO()) as output:
            light.main()
        return json.loads(output.getvalue())

    def test_rejects_world_writable_runtime(self):
        self.runtime.chmod(0o777)
        with self.assertRaises(PermissionError):self.run_light('status','HDMI-A-1')
        self.assertFalse(self.private.exists())

    def test_rejects_symlink_runtime(self):
        self.private.symlink_to(self.root,target_is_directory=True)
        with self.assertRaises(OSError):self.run_light('status','HDMI-A-1')
        self.assertEqual(self.victim.read_text(),'keep this')

    def test_lock_symlink_does_not_truncate_file(self):
        self.private.mkdir(mode=0o700);(self.private/'control.lock').symlink_to(self.victim)
        with self.assertRaises(OSError):self.run_light('status','HDMI-A-1')
        self.assertEqual(self.victim.read_text(),'keep this')

    def test_existing_owned_lock_is_migrated_without_truncation(self):
        self.private.mkdir(mode=0o700);lock=self.private/'control.lock'
        lock.write_text('legacy lock');lock.chmod(0o644)
        self.assertTrue(self.run_light('status','HDMI-A-1')['ok'])
        self.assertEqual(lock.stat().st_mode&0o777,0o600)
        self.assertEqual(lock.read_text(),'legacy lock')

    def test_log_symlink_is_rejected_before_starting_gamma_tool(self):
        self.private.mkdir(mode=0o700)
        _,log=light.paths('HDMI-A-1');log.symlink_to(self.victim)
        with patch.object(light.subprocess,'Popen') as launch:
            with self.assertRaises(OSError):self.run_light('on','HDMI-A-1')
        launch.assert_not_called();self.assertEqual(self.victim.read_text(),'keep this')

    def test_predictable_temporary_symlink_is_not_followed(self):
        self.config.parent.mkdir();self.config.with_suffix('.tmp').symlink_to(self.victim)
        self.assertTrue(self.run_light('off','HDMI-A-1')['ok'])
        self.assertEqual(self.victim.read_text(),'keep this')
        self.assertEqual(json.loads(self.config.read_text())['HDMI-A-1']['enabled'],False)
        self.assertEqual(self.config.stat().st_mode&0o777,0o600)
        self.assertFalse(list(self.config.parent.glob('.night-light-*')))

    def test_forged_record_cannot_stop_an_unrelated_process(self):
        child=subprocess.Popen(['sleep','30'])
        try:
            self.private.mkdir(mode=0o700);path,_=light.paths('HDMI-A-1')
            path.write_text(json.dumps({'pid':child.pid,'args':['sleep','30']}))
            self.assertTrue(self.run_light('off','HDMI-A-1')['ok'])
            self.assertIsNone(child.poll())
        finally:child.terminate();child.wait(timeout=2)

    def test_fifo_state_is_not_read(self):
        self.private.mkdir(mode=0o700);path,_=light.paths('HDMI-A-1');os.mkfifo(path)
        self.assertFalse(self.run_light('status','HDMI-A-1')['enabled'])

    def test_refuses_root(self):
        with patch.object(light.os,'geteuid',return_value=0):
            with self.assertRaises(RuntimeError):light.main()

if __name__=='__main__':unittest.main()
