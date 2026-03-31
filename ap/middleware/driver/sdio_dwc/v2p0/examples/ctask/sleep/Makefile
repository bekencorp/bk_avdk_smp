os = ../../../os/linux
core = ../../../core
build = ../../../build
config = ../../../config
inc = -I../../../inc -I. -I$(build) -I$(config) -I$(os) -I$(core)
objs = main.o
lobjs = $(build)/main.o $(build)/ctask.o $(build)/ctask_no_os.o $(build)/ctask_sem.o $(build)/clog.o $(build)/csem.o

main: $(objs)
	make -C $(core)
	make -C $(os)
	cc $(inc) $(lobjs) -o main -I/usr/lib -I/usr/include -lpthread

main.o: main.c
	cc $(inc) -c main.c -o $(build)/main.o

clean: 
	rm $(build)/main.o main


