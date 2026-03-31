CC = gcc
os = ../os/no_os
core = ../core
build = ../build
config = ../config
test = ../test/core
inc = -I../inc -I. -I$(build) -I$(config) -I$(os) -I$(core) -I$(test) -g -fPIC -Wall
objs = hal.o
robjs = $(build)/hal.o

all: $(objs)

hal.o: hal.c
	$(CC) $(inc) -c hal.c -o $(build)/hal.o

clean: 
	rm $(robjs)


