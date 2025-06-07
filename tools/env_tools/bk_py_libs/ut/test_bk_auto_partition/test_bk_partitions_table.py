from pathlib import Path
from unittest import TestCase

from ut_msic import get_file_md5sum

import bk_auto_partition

curr_dir = Path(__file__).parent


class test_partitions_table(TestCase):
    @classmethod
    def setUpClass(cls): ...

    @classmethod
    def tearDownClass(cls): ...

    def setUp(self) -> None:
        print("")

    def tearDown(self) -> None: ...

    def test_partitions_table_gen_csv_with_crc(self):
        work_space = curr_dir / "workspace/with_crc"
        csv_path = work_space / "auto_partitions.csv"
        gen_csv_path = work_space / "gen_partitions.csv"
        table = bk_auto_partition.bk_partitions_table(csv_path, crc_enable=True)
        table.gen_partition_csv(gen_csv_path)
        self.assertTrue(gen_csv_path.exists(), f"{gen_csv_path} not generate")
        expect_csv_hash = "ff4a79b1d610faa5f2b923b16ad69a28"
        gen_csv_hash = get_file_md5sum(gen_csv_path)
        self.assertEqual(expect_csv_hash, gen_csv_hash)
        gen_csv_path.unlink()

    def test_partitions_table_crc_check(self):
        work_space = curr_dir / "workspace/with_crc"
        csv_path = work_space / "auto_partitions_invalid_crc.csv"
        try:
            bk_auto_partition.bk_partitions_table(csv_path, crc_enable=True)
        except RuntimeError as e:
            self.assertIn("partition align error", str(e))
        else:
            self.assertTrue(False, "not catch error")

    def test_partitions_table_crc_check_overlap(self):
        work_space = curr_dir / "workspace/with_crc"
        csv_path = work_space / "auto_partitions_overlap.csv"
        try:
            bk_auto_partition.bk_partitions_table(csv_path, crc_enable=True)
        except RuntimeError as e:
            self.assertIn("partition table config overlaps", str(e))
        else:
            self.assertTrue(False, "not catch error")

    def test_partitions_table_gen_json_with_crc(self):
        work_space = curr_dir / "workspace/with_crc"
        csv_path = work_space / "auto_partitions.csv"
        gen_json_path = work_space / "gen_partitions.json"
        table = bk_auto_partition.bk_partitions_table(csv_path, crc_enable=True)
        table.gen_partition_json(gen_json_path)
        self.assertTrue(gen_json_path.exists(), f"{gen_json_path} not generate")
        expect_json_hash = "9eb2936bd8e365b692450a32d315cdce"
        gen_json_hash = get_file_md5sum(gen_json_path)
        self.assertEqual(expect_json_hash, gen_json_hash)
        gen_json_path.unlink()

    def test_partitions_table_gen_partitions_show(self):
        work_space = curr_dir / "workspace/with_crc"
        csv_path = work_space / "auto_partitions.csv"
        gen_txt_path = work_space / "gen_partitions.txt"
        table = bk_auto_partition.bk_partitions_table(csv_path, crc_enable=True)
        table.gen_pretty_format_table(gen_txt_path)
        self.assertTrue(gen_txt_path.exists(), f"{gen_txt_path} not generate")
        expect_txt_hash = "7b7487347fe967985d76ab12d3dd33b9"
        gen_txt_hash = get_file_md5sum(gen_txt_path)
        self.assertEqual(expect_txt_hash, gen_txt_hash)
        gen_txt_path.unlink()
