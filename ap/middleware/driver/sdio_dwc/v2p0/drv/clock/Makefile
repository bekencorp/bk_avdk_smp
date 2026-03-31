CC = gcc
os = ../os/csim
core = ../../core
build = ../../build
config = ../../config
terminal = ../../utils/terminal
platform = ../../platform
test = ../test/../core
inc = -I../../inc -I. -I$(platform) -I$(build) -I$(config) -I$(os) -I$(core) -I$(terminal) -I$(test) -g -fPIC -Wall
objs = clkcore.o mmcm_clk.o
robjs = $(build)/clkcore.o $(build)/mmcm_clk.o

all: $(objs)

mmcm_clk.o: mmcm_clk.c
	$(CC) $(inc) -c mmcm_clk.c -o $(build)/mmcm_clk.o

clkcore.o: clkcore.c
	$(CC) $(inc) -c clkcore.c -o $(build)/clkcore.o

clean:
	rm $(robjs)
