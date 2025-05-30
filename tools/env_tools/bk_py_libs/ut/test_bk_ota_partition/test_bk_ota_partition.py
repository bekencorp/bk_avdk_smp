import os
from unittest import TestCase
from ut_msic import get_file_md5sum
from bk_ota_partition import bk_ota_partition

curr_dir = os.path.dirname(__file__)

class test_bk_ota_partition(TestCase):
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

    def test_bk_ota_partition_non_ab(self):
        workdir = f"{curr_dir}/workspace"
        pack_json = f"{workdir}/bk_package.json"
        ota_json = f"{workdir}/bk_ota_partitions.json"
        ota_part = bk_ota_partition(pack_json)
        ota_part.gen_ota_json(ota_json)
        self.assertTrue(os.path.exists(ota_json))
        json_hash = get_file_md5sum(ota_json)
        expect_json_hash = "b9798a16ccfd0b96eb5e1d87c6896669"
        self.assertEqual(expect_json_hash, json_hash)
        os.remove(ota_json)

    def test_bk_ota_partition_ab(self):
        workdir = f"{curr_dir}/workspace"
        pack_json = f"{workdir}/bk_package_ab.json"
        ota_json = f"{workdir}/bk_ota_partitions_ab.json"
        ota_part = bk_ota_partition(pack_json)
        ota_part.gen_ab_ota_json(ota_json)
        self.assertTrue(os.path.exists(ota_json))
        json_hash = get_file_md5sum(ota_json)
        expect_json_hash = "3171046172d17f280527b7986f1fb482"
        self.assertEqual(expect_json_hash, json_hash)
        os.remove(ota_json)
    
    def test_bk_ota_gen_configuration_ab(self):
        workdir = f"{curr_dir}/workspace"
        pack_json = f"{workdir}/bk_package_ab.json"
        config_json = f"{workdir}/bk_configuartion_ab.json"
        ota_part = bk_ota_partition(pack_json)
        ota_part.gen_ab_configuartion_json(config_json)
        self.assertTrue(os.path.exists(config_json))
        json_hash = get_file_md5sum(config_json)
        expect_json_hash = "2ba96898647e75c1f22236318e4ccd9e"
        self.assertEqual(expect_json_hash, json_hash)
        os.remove(config_json)