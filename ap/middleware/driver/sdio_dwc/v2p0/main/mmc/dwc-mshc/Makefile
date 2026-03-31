#os = ../../../os/no_os
CC = gcc
os = ../../../os/linux
core = ../../../core
hal = ../../../platform
build = ../../../build
config = ../../../config
mshc = ../../../drv/mmc/dwc-mshc
mmc = ../../../inc/mmc
platform = ../../../platform/csim_sdlink
rtlsim = ../../../platform/rtlsim
pci = ../../../platform/uiolinux
terminal = ../../../utils/terminal
test = ../../../test/core
utils = ../../../utils/parser
clk = ../../../drv/clock
drvc = ../../../drv/core
inc = -I$(os) -I../../../inc -I. -I$(build) -I$(drvc) -I$(config) -I$(utils) -I$(mshc) -I$(core) -I$(platform) -I$(hal) -I$(mmc) -I$(terminal) -I$(test) -I$(clk) -g -fPIC -Wall -I$(rtlsim) -I/global/apps/vcs_2017.03-SP1/include/
cobj = mshc_main.o mshc_ctest_main.o

ifndef CONFIG_CTASK_LINUX
objs = $(build)/mmc_dev.o $(build)/mmc_core.o $(build)/ctask.o $(build)/cmsg_queue.o $(build)/sdhci.o $(build)/pci_map.o $(build)/csim_link.o $(build)/clink.o $(build)/mshc_main.o $(build)/sdhci_hal.o $(build)/ctask_no_os.o $(build)/ctask_sem.o $(build)/hal.o $(build)/init.o $(build)/fs.o $(build)/fs_linux.o $(build)/clog.o $(build)/terminal.o $(build)/stdcmd.o $(build)/stdio_term.o $(build)/cwatch.o $(build)/cee.o $(build)/rtl_svdpi.o $(build)/mshc_ctest_main.o $(build)/clink_terminal_server.o $(build)/regdesc.o $(build)/sd_cmds.o $(build)/sd_card.o $(build)/delay.o $(build)/emmc_dev.o $(build)/clkcore.o $(build)/mmcm_clk.o $(build)/tee.o $(build)/parser.o $(build)/mmc_test.o
else
objs = $(build)/mmc_dev.o $(build)/mmc_core.o $(build)/ctask.o $(build)/cmsg_queue.o $(build)/sdhci.o $(build)/pci_map.o $(build)/csim_link.o $(build)/clink.o $(build)/mshc_main.o $(build)/sdhci_hal.o $(build)/ctask_linux.o $(build)/ctask_linux_sem.o $(build)/hal.o $(build)/init.o $(build)/fs.o $(build)/fs_linux.o $(build)/clog.o $(build)/terminal.o $(build)/stdcmd.o $(build)/stdio_term.o $(build)/cwatch.o $(build)/cee.o $(build)/rtl_svdpi.o $(build)/mshc_ctest_main.o $(build)/clink_terminal_server.o $(build)/regdesc.o $(build)/sd_cmds.o $(build)/sd_card.o $(build)/delay.o $(build)/emmc_dev.o $(build)/clkcore.o $(build)/mmcm_clk.o $(build)/tee.o $(build)/parser.o $(build)/mmc_test.o
endif

all: $(cobj)
	$(CC) $(inc) $(objs) -o $(build)/mshc_main -I/usr/lib -I/usr/include -lpthread -lpci
	#$(CC) $(inc) $(objs) -shared -o $(build)/ctest_mshc_lib.so -I/usr/lib -I/usr/include -lpthread

mshc_main.o: mshc_main.c
	$(CC) $(inc) -c mshc_main.c -o $(build)/mshc_main.o

mshc_ctest_main.o: mshc_ctest_main.c
	$(CC) $(inc) -c mshc_ctest_main.c -o $(build)/mshc_ctest_main.o
clean: 
	rm $(objs)

