#os = ../../../os
CC = gcc
os = ../../../os/linux
core = ../../../core
build = ../../../build
config = ../../../config
hal = ../../../platform
mmc = ../../../inc/mmc
uio = ../../../platform/uiolinux
test = ../../../test/core
clk = ../../../drv/clock
drvc = ../../../drv/core
parser = ../../../utils/parser
inc = -I$(os) -I../../../inc -I. -I$(clk) -I$(drvc) -I$(build) -I$(config) -I$(hal) -I$(core) -I$(mmc) -I$(uio) -I$(test) -I$(parser) -fPIC -Wall
objs = mmc_dev.o mmc_core.o sdhci_hal.o sdhci.o sd_cmds.o sd_card.o emmc_dev.o mmc_test.o
robjs = $(build)/mmc_dev.o  $(build)/mmc_core.o  $(build)/sdhci_hal.o $(build)/sdhci.o $(build)/sd_cmds.o $(build)/sd_card.o $(build)/emmc_dev.o $(build)/mmc_test.o

all: $(objs)

mmc_dev.o: mmc_dev.c
	$(CC) $(inc) -c mmc_dev.c -o $(build)/mmc_dev.o

sdhci.o: sdhci.c
	$(CC) $(inc) -c sdhci.c -o $(build)/sdhci.o

sdhci_hal.o: sdhci_hal.c
	$(CC) $(inc) -c sdhci_hal.c -o $(build)/sdhci_hal.o

mmc_core.o: mmc_core.c
	$(CC) $(inc) -c mmc_core.c  -o $(build)/mmc_core.o

sd_cmds.o: sd_cmds.c
	$(CC) $(inc) -c sd_cmds.c  -o $(build)/sd_cmds.o

sd_card.o: sd_card.c
	$(CC) $(inc) -c sd_card.c  -o $(build)/sd_card.o

mmc_test.o: mmc_test.c
	$(CC) $(inc) -c mmc_test.c  -o $(build)/mmc_test.o

emmc_dev.o: emmc_dev.c
	$(CC) $(inc) -c emmc_dev.c  -o $(build)/emmc_dev.o

clean: 
	rm $(robjs)

