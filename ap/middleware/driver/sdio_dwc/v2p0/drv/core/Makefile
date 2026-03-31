CC = gcc
os = ../../os/csim
core = ../../core
build = ../../build
config = ../../config
terminal = ../../utils/terminal
mshc_inc = ../../drv/mmc/dwc-mshc/
utils = ../../utils/parser
inc = -I../../inc -I../../inc/mmc/ -I. -I$(build) -I$(config) -I$(os) -I$(core) -I$(terminal) -I$(mshc_inc) -I$(utils) -g -fPIC -Wall
objs = cee.o
robjs = $(build)/cee.o

all: $(objs)

cee.o: cee.c
	$(CC) $(inc) -c cee.c -o $(build)/cee.o
clean:
	rm $(robjs)

