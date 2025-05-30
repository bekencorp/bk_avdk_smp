import os
from unittest import TestCase
from ut_msic import get_file_md5sum
import bk_crc

curr_dir = os.path.dirname(__file__)

class Test_bk_crc16(TestCase):
    @classmethod
    def setUpClass(cls):
        pass

    @classmethod
    def tearDownClass(cls):
        pass
        
    def setUp(self) -> None:
        print("")
        pass

    def tearDown(self) -> None:
        pass

    def test_crc16(self):
        crc = bk_crc.bk_crc16()
        crc.crc_file(f"{curr_dir}/tmp/test1.bin", f"{curr_dir}/tmp/output1.bin")
        gen_file_md5 = get_file_md5sum(f"{curr_dir}/tmp/output1.bin")
        expect_file_md5 = '40b9f4a1d04f271cbb7d479ceea6e37d'
        self.assertEqual(expect_file_md5, gen_file_md5)
        os.remove(f"{curr_dir}/tmp/output1.bin")
    