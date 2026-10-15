# SPDX-License-Identifier: GPL-3.0-or-later
import configparser
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

SCRIPT = Path(__file__).resolve().parents[1]/'scripts/fidan-lock'

class LockPreferencesTest(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory(); self.addCleanup(self.temp.cleanup)
        self.root=Path(self.temp.name); (self.root/'fidan').mkdir()
        tool=self.root/'gtklock'
        tool.write_text("#!/usr/bin/python3\nfrom pathlib import Path\nimport os,sys\nPath(os.environ['CAPTURE']).write_text(Path(sys.argv[2]).read_text())\n")
        tool.chmod(0o700)
        self.env=dict(os.environ,XDG_CONFIG_HOME=str(self.root),XDG_RUNTIME_DIR=str(self.root/'runtime'),PATH=str(self.root)+':'+os.environ['PATH'],CAPTURE=str(self.root/'captured.css'))
        self.desktop=self.root/'desktop image.png'; self.desktop.write_bytes(b'fixture')
        self.lock=self.root/'lock " special image.png'; self.lock.write_bytes(b'fixture')

    def run_lock(self, lock):
        config=configparser.ConfigParser(interpolation=None)
        config.optionxform=str
        config['appearance']={'wallpaper':self.desktop.as_uri(),'lockWallpaper':lock,'accentColor':'#2589ff'}
        with (self.root/'fidan/shell.conf').open('w') as stream: config.write(stream)
        result=subprocess.run(['python3',str(SCRIPT)],env=self.env,capture_output=True,text=True,timeout=4)
        self.assertEqual(result.returncode,0,result.stderr)
        return (self.root/'captured.css').read_text()

    def test_separate_lock_wallpaper_and_accent(self):
        css=self.run_lock(self.lock.as_uri())
        self.assertIn(json.dumps(self.lock.as_uri()),css)
        self.assertIn('#unlock-button { background: #2589ff;',css)

    def test_empty_selection_uses_desktop(self):
        self.assertIn(json.dumps(self.desktop.as_uri()),self.run_lock(''))

    def test_remote_selection_falls_back_to_desktop(self):
        css=self.run_lock('https://example.invalid/image.png')
        self.assertNotIn('https://example.invalid',css)
        self.assertIn(json.dumps(self.desktop.as_uri()),css)

if __name__=='__main__': unittest.main()
