# SPDX-License-Identifier: GPL-3.0-or-later
import contextlib
import hashlib
import importlib.machinery
import importlib.util
import io
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
loader = importlib.machinery.SourceFileLoader('fidan_update', str(ROOT / 'scripts/fidan-update'))
spec = importlib.util.spec_from_loader(loader.name, loader)
update = importlib.util.module_from_spec(spec)
loader.exec_module(update)
RUN = subprocess.run


class UpdaterTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.keys = tempfile.TemporaryDirectory()
        cls.gpg = ['gpg', '--homedir', cls.keys.name, '--batch', '--pinentry-mode', 'loopback', '--passphrase', '']
        RUN(cls.gpg + ['--quick-generate-key', 'Updater Test <test@example.invalid>', 'ed25519', 'sign', '0'], check=True, capture_output=True)
        cls.keyring = Path(cls.keys.name) / 'public.gpg'
        cls.keyring.write_bytes(subprocess.check_output(cls.gpg + ['--export']))

    @classmethod
    def tearDownClass(cls):
        cls.keys.cleanup()

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(); self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.product = {'githubRepo': 'Taxperia/PiOS', 'version': '0.1.0-alpha.1', 'channel': 'alpha', 'updateKeyring': str(self.keyring)}
        package_dir = self.root / 'package/DEBIAN'; package_dir.mkdir(parents=True)
        (package_dir / 'control').write_text('Package: fidan-shell\nVersion: 0.1.0~alpha2-1\nArchitecture: amd64\nMaintainer: Test <test@example.invalid>\nDescription: update fixture\n')
        self.name = 'fidan-shell_0.1.0~alpha2-1_amd64.deb'
        RUN(['dpkg-deb', '--build', str(package_dir.parent), str(self.root / self.name)], check=True, capture_output=True)
        self.manifest = {'schema': 1, 'repository': 'Taxperia/PiOS', 'version': '0.1.0-alpha.2', 'tag': 'v0.1.0-alpha.2',
                         'debianVersion': '0.1.0~alpha2-1', 'packages': [{'name': self.name, 'package': 'fidan-shell', 'architecture': 'amd64',
                         'sha256': hashlib.sha256((self.root / self.name).read_bytes()).hexdigest()}]}
        self.sign()

    def sign(self):
        (self.root / 'release.json').write_text(json.dumps(self.manifest))
        names = ('release.json', self.name)
        (self.root / 'SHA256SUMS').write_text(''.join(hashlib.sha256((self.root / n).read_bytes()).hexdigest() + '  ' + n + '\n' for n in names))
        RUN(self.gpg + ['--yes', '--armor', '--detach-sign', str(self.root / 'SHA256SUMS')], check=True, capture_output=True)

    def release(self, tag='v0.1.0-alpha.2', **kwargs):
        result = {'tag_name': tag, 'prerelease': '-' in tag, 'draft': False, 'assets': [
            {'name': name, 'browser_download_url': f'https://github.com/Taxperia/PiOS/releases/download/{tag}/{name}'}
            for name in ('release.json', 'SHA256SUMS', 'SHA256SUMS.asc', self.name)]}
        result.update(kwargs); return result

    def test_version_order_and_channels(self):
        self.assertTrue(update.newer('0.1.0-alpha.1', 'v0.1.0-alpha.2'))
        self.assertTrue(update.newer('0.1.0-beta.9', 'v0.1.0'))
        self.assertFalse(update.newer('0.1.0', 'v0.1.0-alpha.9'))
        with patch.object(update, 'request_json', return_value=[self.release('v0.1.0-alpha.2'), self.release('v0.1.0-alpha.8'), self.release('v0.2.0-beta.1')]):
            self.assertEqual(update.latest_release(self.product)['tag_name'], 'v0.1.0-alpha.8')

    def test_draft_and_unsafe_tags_are_ignored(self):
        with patch.object(update, 'request_json', return_value=[self.release(draft=True), self.release('v../../evil')]):
            self.assertIsNone(update.latest_release(self.product))

    def test_empty_release_list_is_a_valid_check(self):
        with patch.object(update, 'request_json', return_value=[]), patch.object(update, 'automatic_enabled', return_value=True), contextlib.redirect_stdout(io.StringIO()) as output:
            self.assertEqual(update.command_check(self.product), 0)
        self.assertFalse(json.loads(output.getvalue())['updateAvailable'])

    def test_valid_signed_manifest(self):
        self.assertEqual(update.verified_manifest(self.root, self.product)['version'], '0.1.0-alpha.2')

    def test_installed_update_is_not_offered_again_from_cache(self):
        self.product['version'] = '0.1.0-alpha.2'
        cached = self.root / 'v0.1.0-alpha.2'; cached.mkdir(); (cached / 'ready.json').write_text('{}')
        with patch.object(update, 'latest_release', return_value=self.release()), patch.object(update, 'cache_root', return_value=self.root), patch.object(update, 'automatic_enabled', return_value=True), contextlib.redirect_stdout(io.StringIO()) as output:
            update.command_check(self.product)
        self.assertFalse(json.loads(output.getvalue())['downloaded'])

    def test_modified_manifest_rejected(self):
        (self.root / 'release.json').write_text('{}')
        with self.assertRaises(RuntimeError): update.verified_manifest(self.root, self.product)

    def test_modified_checksums_rejected(self):
        with (self.root / 'SHA256SUMS').open('a') as stream: stream.write('injected\n')
        with self.assertRaises(subprocess.CalledProcessError): update.verified_manifest(self.root, self.product)

    def test_missing_key_rejected(self):
        self.product['updateKeyring'] = str(self.root / 'missing.gpg')
        with self.assertRaises(RuntimeError): update.verified_manifest(self.root, self.product)

    def test_other_repository_rejected(self):
        self.manifest['repository'] = 'Other/Repo'; self.sign()
        with self.assertRaises(RuntimeError): update.verified_manifest(self.root, self.product)

    def test_traversal_filename_rejected(self):
        self.manifest['packages'][0]['name'] = '../package.deb'; self.sign()
        with self.assertRaises(RuntimeError): update.verified_manifest(self.root, self.product)

    def test_incompatible_architecture_rejected(self):
        with patch.object(update, 'architecture', return_value='arm64'):
            with self.assertRaises(RuntimeError): update.compatible_packages(self.manifest)

    def test_complete_download_verifies_assets(self):
        def copy(url, target): shutil.copyfile(self.root / url.rsplit('/', 1)[1], target)
        with patch.object(update, 'latest_release', return_value=self.release()), patch.object(update, 'cache_root', return_value=self.root / 'cache'), patch.object(update, 'download', side_effect=copy), patch.object(update, 'architecture', return_value='amd64'):
            target = update.download_release(self.product)
        self.assertTrue((target / 'ready.json').is_file())

    def test_corrupt_download_does_not_mark_ready(self):
        def copy(url, target):
            shutil.copyfile(self.root / url.rsplit('/', 1)[1], target)
            if target.suffix == '.deb': target.write_bytes(b'corrupt')
        with patch.object(update, 'latest_release', return_value=self.release()), patch.object(update, 'cache_root', return_value=self.root / 'cache'), patch.object(update, 'download', side_effect=copy), patch.object(update, 'architecture', return_value='amd64'):
            with self.assertRaises(RuntimeError): update.download_release(self.product)
        self.assertFalse(list((self.root / 'cache').glob('*/ready.json')))

    def test_install_checks_package_then_calls_apt_without_removal(self):
        calls = []
        def execute(args, **kwargs):
            if args[0] == 'apt-get': calls.append(args); return subprocess.CompletedProcess(args, 0)
            return RUN(args, **kwargs)
        with patch.object(update.os, 'geteuid', return_value=0), patch.object(update, 'architecture', return_value='amd64'), patch.object(update, 'installed_version', return_value='0.1.0~alpha1-1'), patch.object(update.subprocess, 'run', side_effect=execute):
            self.assertEqual(update.install_verified(self.product, self.root), 0)
        self.assertEqual(len(calls), 1); self.assertIn('--no-remove', calls[0])

    def test_install_rejects_corrupt_package_before_apt(self):
        (self.root / self.name).write_bytes(b'corrupt')
        with patch.object(update.os, 'geteuid', return_value=0), patch.object(update, 'architecture', return_value='amd64'), patch.object(update, 'installed_version', return_value='0.1.0~alpha1-1'):
            with self.assertRaisesRegex(RuntimeError, 'Checksum'): update.install_verified(self.product, self.root)

    def test_install_rejects_rollback(self):
        self.product['version'] = '0.2.0'
        with patch.object(update.os, 'geteuid', return_value=0):
            with self.assertRaisesRegex(RuntimeError, 'not newer'): update.install_verified(self.product, self.root)

    def test_asset_urls_cannot_switch_repository(self):
        release = self.release(); release['assets'][0]['browser_download_url'] = 'https://github.com/Other/Repo/releases/download/v0.1.0-alpha.2/release.json'
        self.assertNotIn('release.json', update.asset_map(release, self.product))


if __name__ == '__main__': unittest.main()
