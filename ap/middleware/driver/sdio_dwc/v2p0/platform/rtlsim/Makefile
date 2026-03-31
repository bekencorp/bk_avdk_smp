CC = gcc
os = ../../os/fos
core = ../../core
hal = ../..
build = ../../build
config = ../../config
mshc = ../../drv/mmc/dwc-mshc
msdc = ../../drv/mmc/dwc-msdc
mmc = ../../inc/mmc
platform = ../../platform/csim_sdlink
test = ../../test/core
drvc = ../../drv/core
inc = -I$(os) -I../../inc -I. -I$(build) -I$(drvc) -I$(config) -I$(mshc) -I$(core) -I$(platform) -I$(mmc) -I$(test) -g -fPIC -Wall -I/global/apps/vcs_2017.03-SP1/include/
objs = rtl_svdpi.o 
robjs = $(build)/rtl_svdpi.o

all: $(objs)

rtl_svdpi.o: rtl_svdpi.c
	 $(CC) $(inc) -c rtl_svdpi.c -o $(build)/rtl_svdpi.o

clean:
	rm $(robjs)


