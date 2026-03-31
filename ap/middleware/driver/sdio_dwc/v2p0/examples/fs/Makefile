os = ../../os/no_os
core = ../../core
build = ../../build
config = ../../config
linux = ../../os/linux
inc = -I../../inc -I. -I$(build) -I$(config) -I$(os) -I$(core) -I$(linux)
objs = main.o
lobjs = $(build)/main.o $(build)/ctask.o $(build)/ctask_no_os.o $(build)/ctask_sem.o $(build)/fs.o $(build)/fs_linux.o $(build)/clog.o

main: $(objs)
	make -C $(core)
	make -C $(os)
	make -C $(linux)
	cc $(inc) $(lobjs) -o main

main.o: main.c
	cc $(inc) -c main.c -o $(build)/main.o

clean: 
	rm $(build)/main.o main


