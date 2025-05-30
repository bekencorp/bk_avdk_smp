#!/bin/bash
set -e
current_dir=$(dirname `realpath $0`)
cd $current_dir

python test_bk_crc16/test_main.py
python test_bk_packager/test_main.py
python test_bk_auto_partition/test_main.py
python test_bk_flash_partition/test_main.py
python test_bk_ota_partition/test_main.py
