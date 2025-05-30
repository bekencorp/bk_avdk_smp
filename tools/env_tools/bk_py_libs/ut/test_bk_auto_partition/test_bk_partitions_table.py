import os
from unittest import TestCase
import bk_auto_partition
from ut_msic import get_file_md5sum

curr_dir = os.path.dirname(__file__)

class test_partitions_table(TestCase):
    @classmethod
    def setUpClass(cls): ...

    @classmethod
    def tearDownClass(cls): ...
        
    def setUp(self) -> None:
        print("")

    def tearDown(self) -> None: ...

    def test_partitions_table_gen_csv_with_crc(self):
        work_space = f"{curr_dir}/workspace/with_crc"
        csv_path = f"{work_space}/auto_partitions.csv"
        gen_csv_path = f"{work_space}/gen_partitions.csv"
        table = bk_auto_partition.bk_partitions_table(csv_path, crc_enable=True)
        table.gen_partition_csv(gen_csv_path)
        self.assertTrue(os.path.exists(gen_csv_path), f"{gen_csv_path} not generate")
        expect_csv_hash = "ff4a79b1d610faa5f2b923b16ad69a28"
        gen_csv_hash = get_file_md5sum(gen_csv_path)
        self.assertEqual(expect_csv_hash, gen_csv_hash)
        os.remove(gen_csv_path)
    
    def test_partitions_table_crc_check(self):
        work_space = f"{curr_dir}/workspace/with_crc"
        csv_path = f"{work_space}/auto_partitions_invalid_crc.csv"
        try:
            bk_auto_partition.bk_partitions_table(csv_path, crc_enable=True)
        except RuntimeError as e:
            self.assertIn("partition align error", str(e))
        else:
            self.assertTrue(False, "not catch error")

    def test_partitions_table_crc_check_overlap(self):
        pass
        work_space = f"{curr_dir}/workspace/with_crc"
        csv_path = f"{work_space}/auto_partitions_overlap.csv"
        try:
            bk_auto_partition.bk_partitions_table(csv_path, crc_enable=True)
        except RuntimeError as e:
            self.assertIn("partition table config overlaps", str(e))
        else:
            self.assertTrue(False, "not catch error")

    def test_partitions_table_gen_json_with_crc(self):
        work_space = f"{curr_dir}/workspace/with_crc"
        csv_path = f"{work_space}/auto_partitions.csv"
        gen_json_path = f"{work_space}/gen_partitions.json"
        table = bk_auto_partition.bk_partitions_table(csv_path, crc_enable=True)
        table.gen_partition_json(gen_json_path)
        self.assertTrue(os.path.exists(gen_json_path), f"{gen_json_path} not generate")
        expect_json_hash = "9eb2936bd8e365b692450a32d315cdce"
        gen_json_hash = get_file_md5sum(gen_json_path)
        self.assertEqual(expect_json_hash, gen_json_hash)
        os.remove(gen_json_path)

    def test_partitions_table_gen_partitions_show(self):
        work_space = f"{curr_dir}/workspace/with_crc"
        csv_path = f"{work_space}/auto_partitions.csv"
        gen_txt_path = f"{work_space}/gen_partitions.txt"
        table = bk_auto_partition.bk_partitions_table(csv_path, crc_enable=True)
        table.gen_pretty_format_table(gen_txt_path)
        self.assertTrue(os.path.exists(gen_txt_path), f"{gen_txt_path} not generate")
        expect_txt_hash = "7b7487347fe967985d76ab12d3dd33b9"
        gen_txt_hash = get_file_md5sum(gen_txt_path)
        self.assertEqual(expect_txt_hash, gen_txt_hash)
        os.remove(gen_txt_path)