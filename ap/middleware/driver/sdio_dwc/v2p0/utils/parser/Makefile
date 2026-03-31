CC = gcc
os = ../../os/no_os
core = ../../core
drvc = ../../drv/core
build = ../../build
config = ../../config
test = ../../test/core
inc = -I../../inc -I. -I$(build) -I$(drvc) -I$(config) -I$(os) -I$(core) -I$(test) -g -fPIC -Wall
objs = parser.o
robjs = $(build)/parser.o

all: $(objs)

parser.o: parser.c
	$(CC) $(inc) -c parser.c -o $(build)/parser.o
clean: 
	rm $(robjs)


