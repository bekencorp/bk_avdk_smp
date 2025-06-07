from pathlib import Path
from unittest import TestCase

from ut_msic import get_file_md5sum

from bk_build_summary import bk_build_summary

curr_dir = Path(__file__).parent


class Test_bk_build_summary(TestCase):
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

    def test_build_summary_parititons(self):
        workdir = curr_dir / "workspace"
        partitions_info = workdir / "partitions.txt"
        output_file = workdir / "summary1.txt"
        summary = bk_build_summary()
        summary.set_partitions_info(partitions_info)
        summary.gen_summary(output_file)
        self.assertTrue(output_file.exists())
        expect_file_md5 = "79b21719a6505f0121bf120f07867d56"
        gen_file_md5 = get_file_md5sum(output_file)
        self.assertEqual(expect_file_md5, gen_file_md5)
        output_file.unlink()

    def test_build_summary_app_memory(self):
        workdir = curr_dir / "workspace"
        output_file = workdir / "summary2.txt"
        summary = bk_build_summary()
        summary.set_app_folder("AP", workdir)
        summary.set_app_folder("CP", workdir)
        summary.gen_summary(output_file)
        self.assertTrue(output_file.exists())
        expect_file_md5 = "42762f03af584b1683a8a6fd04aa31c6"
        gen_file_md5 = get_file_md5sum(output_file)
        self.assertEqual(expect_file_md5, gen_file_md5)
        output_file.unlink()

    def test_build_summary_output_file(self):
        workdir = curr_dir / "workspace"
        output_file = workdir / "summary3.txt"
        out_info = "firmware: all_app.bin\n" + "ota binary: ota.rbl\n"
        summary = bk_build_summary()
        summary.set_output_file_info(out_info)
        summary.gen_summary(output_file)
        self.assertTrue(output_file.exists())
        expect_file_md5 = "b136e9f835cd86e9c91ace3efd0ea3a7"
        gen_file_md5 = get_file_md5sum(output_file)
        self.assertEqual(expect_file_md5, gen_file_md5)
        output_file.unlink()

    def test_build_summary_all(self):
        workdir = curr_dir / "workspace"
        partitions_info = workdir / "partitions.txt"
        output_file = workdir / "summary4.txt"
        summary = bk_build_summary()
        summary.set_partitions_info(partitions_info)
        summary.set_app_folder("AP", workdir)
        summary.set_app_folder("CP", workdir)
        out_info = "firmware: all_app.bin\n" + "ota binary: ota.rbl\n"
        summary.set_output_file_info(out_info)
        summary.gen_summary(output_file)
        self.assertTrue(output_file.exists())
        expect_file_md5 = "be488c995c3ccd0c7e350ff885609624"
        gen_file_md5 = get_file_md5sum(output_file)
        self.assertEqual(expect_file_md5, gen_file_md5)
        output_file.unlink()
