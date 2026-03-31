CC = gcc
os = ../../os/csim
core = ../../core
build = ../../build
config = ../../config
mshc = ../../drv/mmc/dwc-mshc
test = ../../test/core
drvc = ../../drv/core
inc = -I../../inc -I. -I$(build) -I$(drvc) -I$(config) -I$(os) -I$(mshc) -I$(core) -I$(test) -g -fPIC -Wall
objs = user-vfio.o
robjs = $(build)/user-vfio.o

all: $(objs)

user-vfio.o: user-vfio.c
	$(CC) $(inc) -c user-vfio.c -o $(build)/user-vfio.o

clean: 
	rm $(robjs)


