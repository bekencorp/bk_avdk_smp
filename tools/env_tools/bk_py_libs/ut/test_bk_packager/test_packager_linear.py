import os
import json
from unittest import TestCase
import bk_packager
from pathlib import Path
from ut_msic import get_file_md5sum

curr_dir = os.path.abspath(os.path.dirname(__file__))

class test_packager_linear(TestCase):
    @classmethod
    def setUpClass(cls): ...

    @classmethod
    def tearDownClass(cls): ...
        
    def setUp(self) -> None:
        print("")

    def tearDown(self) -> None: ...

    def test_packager_wd_not_exist(self):
        no_exist_path = os.path.abspath(f"{curr_dir}/non-exist-workspace")
        try:
            bk_packager.bk_packager_linear(no_exist_path, f"{no_exist_path}/configuartion.json")
        except RuntimeError as e:
            self.assertEqual(f"work directory {no_exist_path} not exist.", str(e))
        else:
            self.assertTrue(False, "not catch error")

    def test_packager_json_not_exist(self):
        workdir = f"{curr_dir}/workspace/common_wd"
        no_exist_json = os.path.abspath(f"{workdir}/non-exist-configuartion.json")
        try:
            bk_packager.bk_packager_linear(workdir, no_exist_json)
        except RuntimeError as e:
            self.assertEqual(f"config json {no_exist_json} not exist.", str(e))
        else:
            self.assertTrue(False, "not catch error")

    def test_packager_json_invalid_format(self):
        workdir = f"{curr_dir}/workspace/test_packager_invalid_json"
        invalid_json = f"{workdir}/configuartion.json"
        try:
            bk_packager.bk_packager_linear(workdir, invalid_json)
        except json.JSONDecodeError:
            pass

    def test_packager_json_invalid_key(self):
        workdir = f"{curr_dir}/workspace/test_packager_json_invalid_key"
        overlap_json = f"{workdir}/configuartion.json"
        Path(f"{workdir}/app.bin").touch()
        Path(f"{workdir}/bootloader.bin").touch()
        try:
            bk_packager.bk_packager_linear(workdir, overlap_json)
        except RuntimeError as e:
            self.assertEqual(f"partition exist overlaps!", str(e))
        else:
            self.assertTrue(False, "not catch error")
        os.remove(f"{workdir}/app.bin")
        os.remove(f"{workdir}/bootloader.bin")

    def test_packager_bin_over_size(self):
        workdir = f"{curr_dir}/workspace/test_packager_oversize"
        over_size_json = f"{workdir}/configuartion.json"
        oversize_bin_path = f"{workdir}/oversize.bin"
        with open(oversize_bin_path, "wb") as f:
            f.write(bytes([0xff] * 256))
        try:
            bk_packager.bk_packager_linear(workdir, over_size_json)
        except RuntimeError as e:
            self.assertIn(f"size is over partitions size.", str(e))
        else:
            self.assertTrue(False, "not catch error")
        os.remove(f"{workdir}/oversize.bin")

    def test_packager_pack(self):
        def prepare():
            bootloader_bytes = bytes([0xff] * 10 + [0x00] * 10)
            cm_app_bytes = bytes([0xff] * 25 + [0x00] * 25)
            ca_app_bytes = bytes([0xff] * 15 + [0x00] * 35)
            with open(f"{workdir}/bootloader.bin", "wb") as f:
                f.write(bootloader_bytes)
            with open(f"{workdir}/cm-app.bin", "wb") as f:
                f.write(cm_app_bytes)
            with open(f"{workdir}/ca-app.bin", "wb") as f:
                f.write(ca_app_bytes)
        def clear():
            os.remove(f"{workdir}/bootloader.bin")
            os.remove(f"{workdir}/cm-app.bin")
            os.remove(f"{workdir}/ca-app.bin")
            os.remove(f"{workdir}/all-app.bin")

        workdir = f"{curr_dir}/workspace/test_packager_linear"
        pack_json = f"{workdir}/configuartion.json"
        prepare()
        packager = bk_packager.bk_packager_linear(workdir, pack_json)
        packager.pack()
        gen_file_md5 = get_file_md5sum(f"{workdir}/all-app.bin")
        expect_file_md5 = 'ccdd7d4b5278ba547e9533c13e6a4bdd'
        self.assertEqual(expect_file_md5, gen_file_md5)
        clear()

    def test_packager_linear_non_zero_start(self):
        def prepare():
            cm_app_bytes = bytes([0xff] * 25 + [0x00] * 25)
            ca_app_bytes = bytes([0xff] * 15 + [0x00] * 35)
            with open(f"{workdir}/cm-app.bin", "wb") as f:
                f.write(cm_app_bytes)
            with open(f"{workdir}/ca-app.bin", "wb") as f:
                f.write(ca_app_bytes)
        def clear():
            os.remove(f"{workdir}/cm-app.bin")
            os.remove(f"{workdir}/ca-app.bin")
            os.remove(f"{workdir}/all-app.bin")

        workdir = f"{curr_dir}/workspace/test_packager_linear_non_zero_start"
        pack_json = f"{workdir}/configuartion.json"
        prepare()
        packager = bk_packager.bk_packager_linear(workdir, pack_json)
        packager.pack()
        gen_file_md5 = get_file_md5sum(f"{workdir}/all-app.bin")
        expect_file_md5 = 'f2e0e2b91bf43f574a738b8d93f5432c'
        self.assertEqual(expect_file_md5, gen_file_md5)
        clear()

    def test_packager_linear_zero_start(self):
        # need fill 0xff from address 0
        def prepare():
            cm_app_bytes = bytes([0xff] * 25 + [0x00] * 25)
            ca_app_bytes = bytes([0xff] * 15 + [0x00] * 35)
            with open(f"{workdir}/cm-app.bin", "wb") as f:
                f.write(cm_app_bytes)
            with open(f"{workdir}/ca-app.bin", "wb") as f:
                f.write(ca_app_bytes)
        def clear():
            os.remove(f"{workdir}/cm-app.bin")
            os.remove(f"{workdir}/ca-app.bin")
            os.remove(f"{workdir}/all-app.bin")

        workdir = f"{curr_dir}/workspace/test_packager_linear_non_zero_start"
        pack_json = f"{workdir}/configuartion.json"
        prepare()
        packager = bk_packager.bk_packager_linear(workdir, pack_json)
        packager.set_start_padding_mode(True)
        packager.pack()
        gen_file_md5 = get_file_md5sum(f"{workdir}/all-app.bin")
        expect_file_md5 = '23913ec21e5429e8e34fe97728407849'
        self.assertEqual(expect_file_md5, gen_file_md5)
        clear()
