# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise the root-only WSL branch with harmless command doubles."""
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

SCRIPT = Path(__file__).resolve().parents[1]/'scripts/run-nested-session'

class NestedLauncherSecurityTest(unittest.TestCase):
    def test_root_permission_changes_use_fresh_root_runtime_entry(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory)
            stub=root/'stub'
            stub.write_text('''#!/usr/bin/python3
import json,os,sys
from pathlib import Path
root=Path(os.environ['LAUNCHER_TEST_ROOT'])
name=Path(sys.argv[0]).name; args=sys.argv[1:]
with (root/'calls').open('a') as stream: stream.write(json.dumps([name,*args])+'\\n')
if name=='id': print('0' if args==['-u'] else '1000')
elif name=='stat': print('555')
elif name=='mktemp':
    if args!=['-d','/run/fidan-x11.XXXXXX']: sys.exit(90)
    fresh=root/'fresh-entry'; fresh.mkdir(mode=0o700); print(fresh)
''')
            stub.chmod(0o700)
            for name in ('id','stat','mktemp','chown','chmod','mount','runuser'):
                (root/name).symlink_to(stub)
            environment=dict(os.environ,PATH=str(root)+':/usr/bin:/bin',LAUNCHER_TEST_ROOT=directory,
                             WSL_DISTRO_NAME='Debian',FIDAN_PRIVATE_MOUNT='1',FIDAN_DESKTOP_USER='fidan')
            # Override only the X11-presence probe so this branch is tested on
            # CI machines without an X server, too. All mutations are doubles.
            command='function [ { if [[ "$1" == -d && "${2:-}" == /tmp/.X11-unix ]]; then return 0; fi; builtin [ "$@"; }; source "$0"'
            result=subprocess.run(['/bin/bash','-c',command,str(SCRIPT)],env=environment,capture_output=True,text=True,timeout=5)
            self.assertEqual(result.returncode,0,result.stderr)
            calls=[json.loads(line) for line in (root/'calls').read_text().splitlines()]
            self.assertIn(['mktemp','-d','/run/fidan-x11.XXXXXX'],calls)
            fresh=str(root/'fresh-entry')
            self.assertIn(['chown','1000:1000',fresh],calls)
            self.assertIn(['chmod','1777',fresh],calls)
            self.assertIn(['mount','--bind',fresh,'/tmp/.X11-unix'],calls)
            self.assertFalse(any('/run/user/1000/' in arg for call in calls for arg in call))

if __name__=='__main__': unittest.main()
