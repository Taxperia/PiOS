#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import importlib.machinery
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import struct
import tempfile
import unittest
from unittest.mock import patch

ROOT=Path(__file__).resolve().parents[1]
def script(name):
    loader=importlib.machinery.SourceFileLoader(name,str(ROOT/'scripts'/name));spec=importlib.util.spec_from_loader(name,loader)
    module=importlib.util.module_from_spec(spec);loader.exec_module(module);return module
build=script('build-image');verify=script('verify-image');hardware=script('test-pi-hardware')
grow=script('fidan-grow-root')


class ImageToolsTests(unittest.TestCase):
    def test_customize_installs_only_manifest_archives_and_generates_offered_locales(self):
        with tempfile.TemporaryDirectory() as directory:
            base=Path(directory); image=base/'image'; hooks=image/'bdebstrap'; hooks.mkdir(parents=True)
            hook=hooks/'customize'; hook.write_bytes((ROOT/'image/bdebstrap/customize').read_bytes())
            packages=image/'packages'; packages.mkdir(); wanted='fidan-artwork_0.1.0~alpha3-1_all.deb'
            (packages/wanted).write_text('verified fixture'); (packages/'unlisted.deb').write_text('must not install')
            # The manifest can come from a Windows checkout.
            (image/'packages.list').write_bytes((wanted+'\r\n').encode())
            target=base/'target'; (target/'etc/greetd').mkdir(parents=True)
            (target/'etc/greetd/fidan.toml').write_text('fixture'); (target/'etc/locale.gen').write_text('# tr_TR.UTF-8 UTF-8\n')
            tools=base/'tools'; tools.mkdir(); log=base/'installed.json'
            stub=tools/'chroot'; stub.write_text('#!/usr/bin/python3\nimport json, os, sys\nfrom pathlib import Path\nif sys.argv[2]=="/bin/sh": Path(os.environ["FIDAN_TEST_LOG"]).write_text(json.dumps(sorted(p.name for p in (Path(sys.argv[1])/"tmp/fidan-packages").iterdir())))\n'); stub.chmod(0o755)
            subprocess.run(['sh',str(hook),str(target)],env=dict(os.environ,PATH=str(tools)+':'+os.environ['PATH'],FIDAN_TEST_LOG=str(log)),check=True)
            self.assertEqual(json.loads(log.read_text()),[wanted]); self.assertFalse((target/'tmp/fidan-packages').exists())
            self.assertIn('\ntr_TR.UTF-8 UTF-8\n',(target/'etc/locale.gen').read_text())
            self.assertIn('\nen_US.UTF-8 UTF-8\n',(target/'etc/locale.gen').read_text())
    def test_root_expansion_refuses_other_filesystems_and_following_partitions(self):
        with tempfile.TemporaryDirectory() as directory:
            disk=Path(directory)/'mmcblk0'; partition=disk/'mmcblk0p2'; partition.mkdir(parents=True)
            (partition/'partition').write_text('2'); device=Path('/dev/mmcblk0p2')
            for current,filesystem in ((Path('/dev/other'),'ext4'),(device,'btrfs')):
                with patch.object(grow.subprocess,'run') as run:
                    with self.assertRaises(RuntimeError):grow.expand(device,current,filesystem,partition,run)
                    run.assert_not_called()
            following=disk/'mmcblk0p3'; following.mkdir(); (following/'partition').write_text('3')
            with patch.object(grow.subprocess,'run') as run:
                with self.assertRaises(RuntimeError):grow.expand(device,device,'ext4',partition,run)
                run.assert_not_called()
    def test_root_expansion_checks_growpart_before_resizing(self):
        with tempfile.TemporaryDirectory() as directory:
            partition=Path(directory)/'mmcblk0/mmcblk0p2'; partition.mkdir(parents=True); (partition/'partition').write_text('2')
            device=Path('/dev/mmcblk0p2')
            for status,output in ((0,'CHANGED'),(1,'NOCHANGE: partition is already full')):
                with self.subTest(status=status),patch.object(grow.subprocess,'run',return_value=subprocess.CompletedProcess([],status,stdout=output)) as run:
                    grow.expand(device,device,'ext4',partition,run)
                    self.assertEqual(run.call_args_list[0].args[0],['growpart','/dev/mmcblk0','2'])
                    self.assertEqual(run.call_args_list[1].args[0],['resize2fs','/dev/mmcblk0p2'])
            with patch.object(grow.subprocess,'run',return_value=subprocess.CompletedProcess([],2,stdout='error')) as run:
                with self.assertRaises(RuntimeError):grow.expand(device,device,'ext4',partition,run)
                self.assertEqual(run.call_count,1)
    def test_cleanup_removes_cloned_identity_and_enables_key_generation(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)/'root'; tools = Path(directory)/'tools'; tools.mkdir()
            for filename in ('etc/ssh/ssh_host_ed25519_key', 'etc/ssh/ssh_host_ed25519_key.pub',
                             'etc/machine-id', 'var/lib/dbus/machine-id', 'var/lib/systemd/random-seed',
                             'usr/lib/systemd/system/greetd.service.d/10-fidan-firstboot.conf',
                             'boot/firmware/initramfs_2712'):
                path = root/filename; path.parent.mkdir(parents=True, exist_ok=True); path.write_text('cloned-identity')
            keep = root/'etc/ssh/sshd_config'; keep.write_text('keep-config')
            for name in ('fidan-firstboot', 'fidan-session', 'fidan-update'):
                path = root/'usr/bin'/name; path.parent.mkdir(parents=True, exist_ok=True); path.write_text('#!/bin/sh\n'); path.chmod(0o755)
            stub = tools/'chroot'; log = Path(directory)/'commands'
            stub.write_text('#!/bin/sh\nprintf "%s\\n" "$*" >> "$FIDAN_TEST_LOG"\ncase "$2" in getent) exit 1 ;; esac\nexit 0\n'); stub.chmod(0o755)
            env = dict(os.environ, PATH=str(tools)+':'+os.environ['PATH'], FIDAN_TEST_LOG=str(log))
            subprocess.run(['sh', str(ROOT/'image/bdebstrap/cleanup-fidan'), str(root)], env=env, check=True)
            self.assertEqual((root/'etc/machine-id').read_bytes(), b'')
            self.assertFalse((root/'var/lib/dbus/machine-id').exists()); self.assertFalse((root/'var/lib/systemd/random-seed').exists())
            self.assertEqual(list((root/'etc/ssh').glob('ssh_host_*')), []); self.assertEqual(keep.read_text(), 'keep-config')
            self.assertIn('enable fidan-ssh-hostkeys.service', log.read_text())
            self.assertIn('mask getty@tty1.service', log.read_text())
            (root/'boot/firmware/initramfs_2712').unlink()
            rejected=subprocess.run(['sh', str(ROOT/'image/bdebstrap/cleanup-fidan'), str(root)], env=env, capture_output=True, text=True)
            self.assertNotEqual(rejected.returncode,0)
            self.assertIn('Missing Pi 5 initramfs',rejected.stderr)
    def test_versions_and_arm64_package_set_are_consistent(self):
        packages,version=build.expected_packages()
        self.assertIn('fidan-remote_'+version+'_all.deb',packages)
        self.assertTrue(all(name.endswith(('_all.deb','_arm64.deb')) for name in packages))
    def test_plan_does_not_download_or_build(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory)
            with patch.object(build.platform,'machine',return_value='x86_64'):
                result=build.plan(root,root/'work',root/'generator')
            self.assertFalse(result['ready']);self.assertFalse((root/'work').exists());self.assertFalse((root/'generator').exists())
            self.assertTrue(any('Missing package' in item for item in result['issues']))
            self.assertEqual(result['generatorCommit'],build.GENERATOR_COMMIT)
    def make_image(self,root):
        image=root/'fixture.img';data=bytearray(128*512)
        data[510:512]=b'\x55\xaa'
        for index,(kind,start,size) in enumerate(((0x0c,8,32),(0x83,40,80))):
            offset=446+16*index;data[offset+4]=kind;struct.pack_into('<II',data,offset+8,start,size)
        data[8*512+54:8*512+62]=b'FAT16   ';data[8*512+510:8*512+512]=b'\x55\xaa';data[40*512+1080:40*512+1082]=b'\x53\xef'
        image.write_bytes(data);return image
    def test_partition_bounds_overlap_and_superblocks(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);image=self.make_image(root)
            self.assertEqual(len(verify.partitions(image)['partitions']),2)
            for offset,value in [(446+16+8,20),(446+16+12,200)]:
                image=self.make_image(root);data=bytearray(image.read_bytes());struct.pack_into('<I',data,offset,value);image.write_bytes(data)
                with self.assertRaises(ValueError):verify.partitions(image)
            image=self.make_image(root);data=bytearray(image.read_bytes());data[40*512+1080]=0;image.write_bytes(data)
            with self.assertRaises(ValueError):verify.partitions(image)
    def test_signature_must_succeed_before_checksum_is_accepted(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);image=self.make_image(root);sums=root/'SHA256SUMS';sums.write_text(build.sha256(image)+'  fixture.img\n')
            with patch.object(verify.subprocess,'run') as run:verify.verify_checksum(image,sums,root/'trusted.gpg');self.assertTrue(run.call_args.kwargs['check'])
            sums.write_text('0'*64+'  fixture.img\n')
            with patch.object(verify.subprocess,'run'):
                with self.assertRaises(ValueError):verify.verify_checksum(image,sums,root/'trusted.gpg')
    def test_no_hardware_does_not_pass_acceptance(self):
        with patch.object(hardware,'read',return_value=''),patch.object(hardware,'probe',return_value={'status':'unavailable'}):report=hardware.collect()
        self.assertFalse(report['pi5']);self.assertFalse(report['hardwareAccepted'])
        self.assertTrue(all(value=='not-performed' for value in report['manualTests'].values()))
        self.assertEqual(report['stress']['status'],'not-performed')


if __name__=='__main__':unittest.main()
