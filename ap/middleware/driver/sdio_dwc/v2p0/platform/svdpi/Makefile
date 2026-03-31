CC = gcc
core = ../../core
os = ../../os/no_os
linux = ../../os/linux
build = ../../build
config = ../../config
terminal = ../../utils/terminal
test = ../../test/core
inc = -I../../inc -I. -I$(build) -I$(config) -I$(os) -I$(terminal) -I$(core) -I$(test)
objs = svdpi_main.o
lobjs = $(build)/terminal.o $(build)/stdcmd.o $(build)/stdio_term.o $(build)/svdpi_main.o $(build)/ctask.o $(build)/csem.o $(build)/init.o $(build)/ctask_no_os.o $(build)/ctask_sem.o $(build)/cmsg_queue.o $(build)/clog.o $(build)/fs_linux.o $(build)/fs.o $(build)/cwatch.o $(build)/clink.o $(build)/ctask_linux.o $(build)/ctask_linux_sem.o $(build)/cee.o

svdpi_main: $(objs)
	make -C $(os)
	make -C $(linux)
	make -C $(core)
	make -C $(test)
	make -C $(terminal)
	cc $(inc) $(lobjs) -o svdpi_main -I/usr/lib -I/usr/include -lpthread

svdpi_main.o: svdpi_main.c
	cc $(inc) -c svdpi_main.c -o $(build)/svdpi_main.o -I/usr/lib -I/usr/include

clean: 
	rm $(lobjs) svdpi_main


