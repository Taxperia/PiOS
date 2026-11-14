import configparser
import importlib.machinery
import importlib.util
from pathlib import Path
import tempfile
import unittest

loader=importlib.machinery.SourceFileLoader('setup_preferences',str(Path(__file__).resolve().parents[1]/'scripts/fidan-setup-preferences'))
spec=importlib.util.spec_from_loader('setup_preferences',loader);module=importlib.util.module_from_spec(spec);loader.exec_module(module)


class SetupPreferencesTests(unittest.TestCase):
    def values(self):return dict(themeMode='light',accentColor='#2589ff',dockPosition='bottom',panelPosition='top',panelStyle='compact',dateFormat='day-first',language='tr_TR.UTF-8',keyboardLayout='tr',reducedMotion=True)
    def test_account_preferences_finish_the_same_welcome_and_preserve_other_settings(self):
        with tempfile.TemporaryDirectory() as home:
            target=Path(home)/'.config/fidan/shell.conf';target.parent.mkdir(parents=True);target.write_text('[audio]\nvolume=42\n')
            module.save(home,self.values());config=configparser.ConfigParser();config.read(target)
            self.assertTrue(config.getboolean('welcome','completed'));self.assertEqual(config['audio']['volume'],'42')
            self.assertEqual(config['appearance']['thememode'],'light');self.assertEqual(config['region']['language'],'tr_TR.UTF-8')
            self.assertIn('language=tr_TR.UTF-8\n',target.read_text())
            self.assertIn('keyboardLayout=tr\n',target.read_text())
            self.assertEqual(target.stat().st_mode&0o777,0o600)
    def test_unknown_fields_and_linked_destination_never_write(self):
        with tempfile.TemporaryDirectory() as home:
            values=self.values();values['command']='untrusted'
            with self.assertRaises(ValueError):module.save(home,values)
            target=Path(home)/'.config/fidan/shell.conf';target.parent.mkdir(parents=True);other=Path(home)/'keep';other.write_text('keep')
            target.symlink_to(other)
            with self.assertRaises(ValueError):module.save(home,self.values())
            self.assertEqual(other.read_text(),'keep')
    def test_green_and_black_are_distinct_new_profile_choices(self):
        with tempfile.TemporaryDirectory() as home:
            for mode in ('green','dark'):
                values=self.values();values['themeMode']=mode;module.save(home,values)
                config=configparser.ConfigParser();config.read(Path(home)/'.config/fidan/shell.conf')
                self.assertEqual(config['appearance']['thememode'],mode)
                self.assertEqual(config['appearance']['themeschema'],'2')
                self.assertTrue(config.getboolean('appearance','darktheme'))


if __name__=='__main__':unittest.main()
