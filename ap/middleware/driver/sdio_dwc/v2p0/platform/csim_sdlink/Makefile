CC = gcc
os = ../../os/fos
core = ../../core
hal = ../..
build = ../../build
config = ../../config
mshc = ../../drv/mmc/dwc-mshc
msdc = ../../drv/mmc/dwc-msdc
mmc = ../../inc/mmc
drvc = ../../drv/core
platform = ../../platform/csim_sdlink
test = ../../test/core
inc = -I$(os) -I../../inc -I. -I$(drvc) -I$(build) -I$(config) -I$(mshc) -I$(core) -I$(platform) -I$(mmc) -I$(test) -g -fPIC -Wall
objs = csim_link.o 
robjs = $(build)/csim_link.o 

all: $(objs)

csim_link.o: csim_link.c
	 $(CC) $(inc) -c csim_link.c -o $(build)/csim_link.o

clean: 
	rm $(robjs)


