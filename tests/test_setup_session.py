#!/usr/bin/python3
"""Exercise the installed session entry point without starting a compositor."""
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]


class SetupSessionTests(unittest.TestCase):
    def test_setup_runtime_uses_service_uid_with_no_physical_inputs(self):
        with tempfile.TemporaryDirectory() as directory:
            base=Path(directory); tools=base/'tools'; tools.mkdir(); report=base/'environment.json'
            commands={
                'id':'#!/bin/sh\nprintf "1234\\n"\n',
                'ls':'#!/bin/sh\nexit 0\n',
                'fidan-input':'#!/bin/sh\nprintf "%s\\n" "$SETUP_SESSION_TEST_PROFILE"\n',
                'labwc':'#!/usr/bin/python3\nimport json,os\nfrom pathlib import Path\nPath(os.environ["SETUP_SESSION_TEST_REPORT"]).write_text(json.dumps({k:os.environ.get(k) for k in ("XDG_RUNTIME_DIR","WLR_BACKENDS","WLR_LIBINPUT_NO_DEVICES")}))\n',
            }
            for name,content in commands.items():
                path=tools/name;path.write_text(content);path.chmod(0o755)
            env=dict(os.environ,PATH=str(tools)+':/usr/bin:/bin',HOME=str(base/'home'),XDG_RUNTIME_DIR='/run/user/0',
                     SETUP_SESSION_TEST_REPORT=str(report),SETUP_SESSION_TEST_PROFILE=str(base/'profile'))
            for key in ('WLR_BACKENDS','WLR_LIBINPUT_NO_DEVICES','XDG_CONFIG_HOME'): env.pop(key,None)
            subprocess.run(['sh',str(ROOT/'scripts/fidan-session'),'--setup'],env=env,check=True,timeout=5)
            data=json.loads(report.read_text())
            self.assertEqual(data['XDG_RUNTIME_DIR'],'/run/user/1234')
            self.assertEqual(data['WLR_BACKENDS'],'drm,libinput,headless')
            self.assertEqual(data['WLR_LIBINPUT_NO_DEVICES'],'1')


if __name__=='__main__':unittest.main()
