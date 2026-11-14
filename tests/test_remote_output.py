#!/usr/bin/python3
"""Validate headless defaults without touching real displays or accounts."""
import importlib.machinery
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

ROOT=Path(__file__).resolve().parents[1]
loader=importlib.machinery.SourceFileLoader('remote_output',str(ROOT/'scripts/fidan-remote-session'))
spec=importlib.util.spec_from_loader('remote_output',loader);output=importlib.util.module_from_spec(spec);loader.exec_module(output)


class RemoteOutputTests(unittest.TestCase):
    def prepare(self,outputs,profile=None):
        with tempfile.TemporaryDirectory() as home:
            if profile is not None:
                file=Path(home)/'fidan/displays.json';file.parent.mkdir();file.write_text(json.dumps(profile))
            with patch.dict(os.environ,{'XDG_CONFIG_HOME':home}),patch.object(output.os,'geteuid',return_value=1000),patch.object(output.subprocess,'run') as run:
                run.return_value=subprocess.CompletedProcess([],0,stdout=json.dumps(outputs))
                output.prepare();return run.call_args_list
    def test_default_is_readable_with_less_pixel_and_refresh_work(self):
        calls=self.prepare([{'name':'HEADLESS-1','enabled':True}])
        self.assertEqual(calls[-1].args[0],['wlr-randr','--output','HEADLESS-1','--on','--custom-mode','1600x900@30','--scale','1.25'])
    def test_attached_monitor_keeps_its_settings_and_disables_virtual_output(self):
        calls=self.prepare([{'name':'HEADLESS-1','enabled':True},{'name':'HDMI-A-1','enabled':True}])
        self.assertEqual(calls[-1].args[0],['wlr-randr','--output','HEADLESS-1','--off'])
    def test_confirmed_profile_is_not_overwritten(self):
        calls=self.prepare([{'name':'HEADLESS-1','enabled':True}],{'HEADLESS-1':{'width':1920,'height':1080,'scale':1.5}})
        self.assertEqual(len(calls),1)
    def test_no_virtual_output_does_not_change_any_display(self):
        self.assertEqual(len(self.prepare([{'name':'HDMI-A-1','enabled':True}])),1)
    def test_empty_profile_does_not_leave_headless_defaults_unconfigured(self):
        self.assertEqual(len(self.prepare([{'name':'HEADLESS-1','enabled':True}],{'HEADLESS-1':{}})),2)


if __name__=='__main__':unittest.main()
