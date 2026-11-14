#!/usr/bin/python3
"""Exercise screenshot targets and cancellation without accessing a real display."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

SCRIPT = Path(__file__).resolve().parents[1] / 'scripts/fidan-screenshot'

class ScreenshotTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.tools = self.root / 'bin'
        self.tools.mkdir()
        self.env = dict(os.environ, PATH=str(self.tools) + ':' + os.environ['PATH'],
            FIDAN_CAPTURE_ROOT=str(self.root))
        self.tool('xdg-user-dir', 'printf "%s/Pictures\\n" "$FIDAN_CAPTURE_ROOT"')
        self.tool('notify-send', 'exit 0')
        self.tool('slurp', '[ "${FIDAN_CAPTURE_CANCEL:-0}" != 1 ] || exit 1\nprintf "12,34 500x300\\n"')
        self.tool('grim', '''printf '%s\\n' "$*" > "$FIDAN_CAPTURE_ROOT/arguments"
[ "${FIDAN_CAPTURE_FAIL:-0}" != 1 ] || exit 1
for target do :; done
printf 'screenshot bytes' > "$target"''')
        self.tool('wl-copy', 'cat > "$FIDAN_CAPTURE_ROOT/clipboard"')

    def tool(self, name, source):
        file = self.tools / name
        file.write_text('#!/bin/sh\nset -eu\n' + source + '\n')
        file.chmod(0o700)

    def run_capture(self, *args):
        return subprocess.run(['sh', str(SCRIPT), *args], env=self.env,
            capture_output=True, text=True, timeout=5)

    def test_targets_and_clipboard(self):
        for mode in ['--screen', '--region', '--window']:
            with self.subTest(mode=mode):
                result = self.run_capture(mode)
                self.assertEqual(result.returncode, 0, result.stderr)
                image = Path(result.stdout.strip())
                self.assertEqual(image.read_bytes(), b'screenshot bytes')
                self.assertEqual((self.root / 'clipboard').read_bytes(), b'screenshot bytes')
                self.assertEqual('-g 12,34 500x300' in (self.root / 'arguments').read_text(), mode != '--screen')

    def test_cancel_removes_empty_file(self):
        self.env['FIDAN_CAPTURE_CANCEL'] = '1'
        result = self.run_capture('--window')
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(list((self.root / 'Pictures/Screenshots').glob('*.png')), [])
        self.assertFalse((self.root / 'clipboard').exists())

    def test_capture_failure_removes_empty_file(self):
        self.env['FIDAN_CAPTURE_FAIL'] = '1'
        self.assertNotEqual(self.run_capture('--screen').returncode, 0)
        self.assertEqual(list((self.root / 'Pictures/Screenshots').glob('*.png')), [])

    def test_invalid_target_has_no_side_effect(self):
        self.assertEqual(self.run_capture('--invalid').returncode, 2)
        self.assertFalse((self.root / 'Pictures').exists())

if __name__ == '__main__':
    unittest.main()
