CC = gcc
os = ../../os/csim
core = ../../core
build = ../../build
config = ../../config
mshc = ../../drv/mmc/dwc-mshc
test = ../../test/core
drvc = ../../drv/core
inc = -I../../inc -I. -I$(build) -I$(drvc) -I$(config) -I$(os) -I$(mshc) -I$(core) -I$(test) -g -fPIC -Wall -I/usr/include
objs = pci_map.o
robjs = $(build)/pci_map.o

all: $(objs)

pci_map.o: pci_map.c
	$(CC) $(inc) -c pci_map.c -o $(build)/pci_map.o

clean: 
	rm $(robjs)


