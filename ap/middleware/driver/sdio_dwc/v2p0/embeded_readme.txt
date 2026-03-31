****************************************************
* DWC Mobile Storage CTest Sample Software         *
* For Designware DWC MSHC IP                       *
*     DWC mshc Driver Version 2.0 - 01/2025        *
****************************************************

test command line:
sdio_host -d 6 --speed_mode 0 --xfer_mode 0 --bus_width 1 --mmcm_clk 160 --emmc 0 --stop_at_cmd 0 --mmcm_clk 160
sdio_usr_intf wr 0 1
sdio_usr_intf wr 0 2
sdio_usr_intf wr 0 4
sdio_usr_intf wr 0 8
sdio_usr_intf rd 0 1
sdio_usr_intf rd 0 2
sdio_usr_intf rd 0 4
sdio_usr_intf rd 0 8

