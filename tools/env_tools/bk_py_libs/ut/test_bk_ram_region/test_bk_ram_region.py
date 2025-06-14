from pathlib import Path
from unittest import TestCase

from ut_msic import get_file_md5sum

from bk_ram_region import bk_ram_region

curr_dir = Path(__file__).parent


class test_bk_ram_region(TestCase):
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

    def test_bk_ram_region_partitions(self):
        workdir = curr_dir / "workspace"
        mem_csv = workdir / "ram_region.csv"
        mem_header = workdir / "ram_region.h"
        ram_region = bk_ram_region(mem_csv)
        ram_region.gen_memory_layout_hdr(mem_header)
        self.assertTrue(mem_header.exists())
        gen_file_md5 = get_file_md5sum(mem_header)
        expected_file_md5 = "c2224b2967709e5a61f6df407963ddbc"
        self.assertEqual(expected_file_md5, gen_file_md5)
        mem_header.unlink()

    def test_bk_ram_region_only_sram(self):
        workdir = curr_dir / "workspace"
        mem_csv = workdir / "ram_region_only_sram.csv"
        mem_header = workdir / "ram_region_only_sram.h"
        ram_region = bk_ram_region(mem_csv)
        ram_region.gen_memory_layout_hdr(mem_header)
        self.assertTrue(mem_header.exists())
        gen_file_md5 = get_file_md5sum(mem_header)
        expected_file_md5 = "5ceac0bf68b7834c03c267573bbbec8f"
        self.assertEqual(expected_file_md5, gen_file_md5)
        mem_header.unlink()

    def test_bk_ram_region_only_psram(self):
        workdir = curr_dir / "workspace"
        mem_csv = workdir / "ram_region_only_psram.csv"
        mem_header = workdir / "ram_region_only_psram.h"
        ram_region = bk_ram_region(mem_csv)
        ram_region.gen_memory_layout_hdr(mem_header)
        self.assertTrue(mem_header.exists())
        gen_file_md5 = get_file_md5sum(mem_header)
        expected_file_md5 = "085911308dcc8cda98744ba7b682f8fa"
        self.assertEqual(expected_file_md5, gen_file_md5)
        mem_header.unlink()
