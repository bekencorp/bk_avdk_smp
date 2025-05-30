import os
from unittest import TestCase
from ut_msic import get_file_md5sum
from bk_flash_partiton import bk_flash_partition

curr_dir = os.path.dirname(__file__)

class test_bk_flash_partition(TestCase):
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

    def test_flash_partition_src(self):
        workdir = f"{curr_dir}/workspace"
        partition_json = f"{workdir}/gen_partitions.json"
        flash_part = bk_flash_partition(partition_json)
        header_path = f"{workdir}/vendor_flash_partition.h"
        src_path = f"{workdir}/vendor_flash.c"
        flash_part.gen_flash_partitions_src(header_path, src_path)
        self.assertTrue(os.path.exists(header_path))
        self.assertTrue(os.path.exists(src_path))
        header_hash = get_file_md5sum(header_path)
        expect_header_hash = "08ffdf8c98038f602410ddebdcdf9154"
        self.assertEqual(expect_header_hash, header_hash)
        src_hash = get_file_md5sum(src_path)
        expect_src_hash = "c503a1fae43f5713e7bca57412cdaf3d"
        self.assertEqual(expect_src_hash, src_hash)
        os.remove(header_path)
        os.remove(src_path)

    def test_gen_partitions_header(self):
        workdir = f"{curr_dir}/workspace"
        partition_json = f"{workdir}/gen_partitions.json"
        flash_part = bk_flash_partition(partition_json)
        header_path = f"{workdir}/partitions.h"
        flash_part.gen_partitions_layout_hdr(header_path)
        self.assertTrue(os.path.exists(header_path))
        header_hash = get_file_md5sum(header_path)
        expect_header_hash = "66a995904a8b9dae323b42b7ff9e0f67"
        self.assertEqual(expect_header_hash, header_hash)
        os.remove(header_path)

    def test_gen_pack_json(self):
        workdir = f"{curr_dir}/workspace"
        partition_json = f"{workdir}/gen_partitions.json"
        package_json = f"{workdir}/bk_package.json"
        flash_part = bk_flash_partition(partition_json)
        flash_part.gen_pack_json(package_json)
        self.assertTrue(os.path.exists(package_json))
        json_hash = get_file_md5sum(package_json)
        expect_header_hash = "e38787543531c8373d6e01431f398b17"
        self.assertEqual(expect_header_hash, json_hash)
        os.remove(package_json)
