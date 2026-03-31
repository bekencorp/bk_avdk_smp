os = ../../os/no_os
test = ../../test/core
core = ../../core
helper = ../../utils/parser
build = ../../build
config = ../../config
linux = ../../os/linux
inc = -I../../inc -I. -I$(build) -I$(config) -I$(os) -I$(core) -I$(linux) -I$(helper) -I$(test)
objs = main.o
lobjs = $(build)/main.o $(build)/parser.o $(build)/clog.o

main: $(objs)
	make -C $(core)
	cc $(inc) $(lobjs) -o main

main.o: main.c
	cc $(inc) -c main.c -o $(build)/main.o

clean: 
	rm $(build)/main.o main


