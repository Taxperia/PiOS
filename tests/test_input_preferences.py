# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise personal Labwc updates in isolated directories, without a compositor."""
import importlib.machinery
import importlib.util
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import xml.etree.ElementTree as ET

SCRIPT=Path(__file__).resolve().parents[1]/'scripts/fidan-input'
loader=importlib.machinery.SourceFileLoader('fidan_input',str(SCRIPT))
spec=importlib.util.spec_from_loader(loader.name,loader)
module=importlib.util.module_from_spec(spec)
loader.exec_module(module)

class InputTest(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root=Path(self.temp.name)
        self.config=self.root/'config'
        (self.config/'fidan').mkdir(parents=True)
        self.profile=self.config/'labwc'
        self.profile.mkdir()
        self.xml=self.profile/'rc.xml'
        self.xml.write_text('<labwc_config><keyboard><keybind key="W-r"><action name="Execute" command="fidan-apps"/></keybind></keyboard><theme><cornerRadius>8</cornerRadius></theme></labwc_config>')

    def test_applies_preferences_and_preserves_unrelated_config(self):
        (self.config/'fidan/input.conf').write_text('[General]\npointerSpeed=0.4\nrepeatRate=37\nrepeatDelay=450\nleftHanded=true\nnaturalScroll=true\ntap=false\n')
        module.apply_xml(self.xml,module.settings(self.config))
        root=ET.parse(self.xml).getroot()
        self.assertEqual(root.findtext('keyboard/repeatRate'),'37')
        self.assertEqual(root.findtext('keyboard/repeatDelay'),'450')
        self.assertEqual(root.find('keyboard/keybind/action').get('command'),'fidan-apps')
        self.assertEqual(root.findtext('theme/cornerRadius'),'8')
        self.assertEqual(root.findtext("libinput/device[@category='default']/pointerSpeed"),'0.4')
        self.assertEqual(root.findtext("libinput/device[@category='default']/leftHanded"),'yes')
        self.assertEqual(root.findtext("libinput/device[@category='touchpad']/tap"),'no')
        self.assertEqual(self.xml.stat().st_mode&0o777,0o600)
        self.assertEqual(list(self.profile.glob('.input-*')),[])

    def test_invalid_values_do_not_change_xml(self):
        original=self.xml.read_bytes()
        for key,value in [('repeatRate','0'),('repeatDelay','9999'),('pointerSpeed','nan'),('pointerSpeed','2')]:
            with self.subTest(key=key,value=value):
                (self.config/'fidan/input.conf').write_text(f'[General]\n{key}={value}\n')
                with self.assertRaises(ValueError): module.settings(self.config)
                self.assertEqual(self.xml.read_bytes(),original)

    def test_apply_uses_personal_profile_and_requests_reconfigure(self):
        env={'XDG_CONFIG_HOME':str(self.config),'FIDAN_LABWC_CONFIG':str(self.profile)}
        with patch.dict(os.environ,env),patch.object(module.os,'geteuid',return_value=1000),patch.object(module.sys,'argv',['fidan-input','apply']),patch.object(module.subprocess,'run') as run:
            module.main()
        run.assert_called_once_with(['labwc','--reconfigure'],timeout=5,check=True)
        self.assertEqual(ET.parse(self.xml).getroot().findtext('keyboard/repeatRate'),'25')

    def test_apply_refuses_profile_outside_user_roots(self):
        outside=self.root/'system'
        outside.mkdir()
        (outside/'rc.xml').write_bytes(self.xml.read_bytes())
        with patch.dict(os.environ,{'FIDAN_LABWC_CONFIG':str(outside),'LABWC_CONFIG_DIR':'','XDG_RUNTIME_DIR':str(self.root/'runtime')}),patch.object(module.Path,'iterdir',return_value=iter([])):
            with self.assertRaises(RuntimeError): module.active_directory(self.config)

    def test_nested_startup_applies_saved_settings_without_reconfigure(self):
        (self.config/'fidan/input.conf').write_text('[General]\nrepeatRate=40\n')
        env={'XDG_CONFIG_HOME':str(self.config),'FIDAN_LABWC_CONFIG':str(self.profile)}
        with patch.dict(os.environ,env),patch.object(module.os,'geteuid',return_value=1000),patch.object(module.sys,'argv',['fidan-input','configure']),patch.object(module.subprocess,'run') as run:
            module.main()
        run.assert_not_called()
        self.assertEqual(ET.parse(self.xml).getroot().findtext('keyboard/repeatRate'),'40')

    def test_refuses_root(self):
        with patch.object(module.os,'geteuid',return_value=0):
            with self.assertRaises(RuntimeError): module.main()

if __name__=='__main__': unittest.main()
