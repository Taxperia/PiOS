# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise saved display restoration without touching the running compositor."""
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

RESTORE = Path(__file__).resolve().parents[1] / 'scripts/fidan-display-restore'

class DisplayRestoreTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.config = self.root / 'fidan'
        self.config.mkdir()
        self.profile = dict(name='HDMI-A-1', width=1920, height=1080, refresh=60, scale=1.25, transform='normal')
        output = dict(name='HDMI-A-1', enabled=True, modes=[dict(width=1920, height=1080, refresh=60)])
        (self.root/'outputs').write_text(json.dumps([output]))
        tool = self.root/'wlr-randr'
        tool.write_text("#!/usr/bin/python3\nimport json,pathlib,sys\np=pathlib.Path(__file__).parent\na=sys.argv[1:]\nwith (p/'commands').open('a') as f: f.write(json.dumps(a)+'\\n')\nif '--json' in a: print((p/'outputs').read_text())\nif '--dryrun' in a and (p/'reject').exists(): sys.exit(1)\n")
        tool.chmod(0o700)
        self.env = dict(os.environ, XDG_CONFIG_HOME=str(self.root), PATH=str(self.root)+':'+os.environ['PATH'])

    def run_restore(self, profiles):
        (self.config/'displays.json').write_text(json.dumps(profiles))
        result = subprocess.run([str(RESTORE)], env=self.env, capture_output=True, text=True, timeout=6)
        self.assertEqual(result.returncode, 0)
        self.assertNotIn('Traceback', result.stderr)
        path = self.root/'commands'
        return [json.loads(line) for line in path.read_text().splitlines()] if path.exists() else []

    def test_restores_confirmed_advertised_mode_after_dryrun(self):
        calls = self.run_restore({'HDMI-A-1': self.profile})
        self.assertEqual(len(calls), 3)
        self.assertEqual(calls[1], ['--dryrun', *calls[2]])
        self.assertIn('1920x1080@60.000Hz', calls[2])
        self.assertNotIn('--off', calls[2])

    def test_rejected_mode_is_not_applied(self):
        (self.root/'reject').touch()
        self.assertEqual(len(self.run_restore({'HDMI-A-1': self.profile})), 2)

    def test_unadvertised_mode_is_skipped(self):
        self.profile['width'] = 9999
        self.assertEqual(self.run_restore({'HDMI-A-1': self.profile}), [['--json']])

    def test_invalid_scale_is_skipped(self):
        self.profile['scale'] = float('nan')
        self.assertEqual(self.run_restore({'HDMI-A-1': self.profile}), [['--json']])

    def test_virtual_custom_mode_is_validated_before_restore(self):
        (self.root/'outputs').write_text(json.dumps([dict(name='WL-1',enabled=True,modes=[])]))
        self.profile.update(name='WL-1',width=2560,height=1440,custom=True)
        calls=self.run_restore({'WL-1':self.profile})
        self.assertEqual(len(calls),3)
        self.assertEqual(calls[1],['--dryrun',*calls[2]])
        self.assertIn('--custom-mode',calls[2])
        self.assertIn('2560x1440@60.000Hz',calls[2])

    def test_custom_mode_never_bypasses_physical_monitor_modes(self):
        self.profile.update(width=3840,height=2160,custom=True)
        self.assertEqual(self.run_restore({'HDMI-A-1':self.profile}),[['--json']])

    def test_nested_wayland_uses_parent_refresh_without_zero_hz_argument(self):
        (self.root/'outputs').write_text(json.dumps([dict(name='WL-1',enabled=True,modes=[])]))
        self.profile.update(name='WL-1',width=1600,height=900,refresh=0,custom=True)
        calls=self.run_restore({'WL-1':self.profile})
        self.assertEqual(len(calls),3)
        self.assertIn('1600x900',calls[2])
        self.assertFalse(any('@0' in arg for arg in calls[2]))

    def test_virtual_custom_mode_outside_bounds_is_skipped(self):
        (self.root/'outputs').write_text(json.dumps([dict(name='WL-1',enabled=True,modes=[])]))
        self.profile.update(name='WL-1',width=9999,custom=True)
        self.assertEqual(self.run_restore({'WL-1':self.profile}),[['--json']])

    def test_malformed_profile_is_handled(self):
        self.assertEqual(self.run_restore([]), [])

if __name__ == '__main__':
    unittest.main()
