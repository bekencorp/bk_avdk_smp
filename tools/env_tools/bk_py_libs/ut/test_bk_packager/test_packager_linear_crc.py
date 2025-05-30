import os
from unittest import TestCase
import bk_packager
from ut_msic import get_file_md5sum

curr_dir = os.path.dirname(__file__)

class test_packager_linear_crc(TestCase):
    @classmethod
    def setUpClass(cls): ...

    @classmethod
    def tearDownClass(cls): ...
        
    def setUp(self) -> None:
        print("")

    def tearDown(self) -> None: ...

    def test_packager_with_crc_pack(self):
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

        workdir = f"{curr_dir}/workspace/test_packager_linear_crc"
        pack_json = f"{workdir}/configuartion.json"
        prepare()
        packager = bk_packager.bk_packager_linear_crc(workdir, pack_json)
        packager.pack()
        gen_file_md5 = get_file_md5sum(f"{workdir}/all-app.bin")
        expect_file_md5 = '11cb4d1758f6dee78d10cf7ba04f1364'
        self.assertEqual(expect_file_md5, gen_file_md5)
        clear()

    