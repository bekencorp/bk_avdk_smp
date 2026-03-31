CC = gcc
os = ../../os/csim
core = ../../core
build = ../../build
config = ../../config
terminal = ../../utils/terminal
mshc_inc = ../../drv/mmc/dwc-mshc/
utils = ../../utils/parser
drvc = ../../drv/core
inc = -I../../inc -I../../inc/mmc/ -I. -I$(drvc) -I$(build) -I$(config) -I$(os) -I$(core) -I$(terminal) -I$(mshc_inc) -I$(utils) -g -fPIC -Wall
objs = tee.o
robjs = $(build)/tee.o

all: $(objs)

tee.o: tee.c
	$(CC) $(inc) -c tee.c -o $(build)/tee.o
clean:
	rm $(robjs)

