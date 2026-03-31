core = ../../../core
os = ../../../os/no_os
linux = ../../../os/linux
build = ../../../build
config = ../../../config
terminal = ../../../utils/terminal
test = ../../../test/core
inc = -I../../../inc -I. -I$(build) -I$(config) -I$(os) -I$(terminal) -I$(core) -I$(test)
objs = main.o
lobjs = $(build)/terminal.o $(build)/stdcmd.o $(build)/stdio_term.o $(build)/main.o $(build)/ctask.o $(build)/csem.o $(build)/init.o $(build)/ctask_no_os.o $(build)/ctask_sem.o $(build)/cmsg_queue.o $(build)/clog.o $(build)/fs_linux.o $(build)/fs.o $(build)/cwatch.o $(build)/clink.o $(build)/cee.o $(build)/hal.o $(build)/regdesc.o $(build)/clkcore.o $(build)/mmcm_clk.o

main: $(objs)
	make -C $(os)
	make -C $(test)
	make -C $(linux)
	make -C $(core)
	make -C $(terminal)
	cc $(inc) $(lobjs) -o main -I/usr/lib -I/usr/include -lpthread

main.o: main.c
	cc $(inc) -c main.c -o $(build)/main.o -I/usr/lib -I/usr/include

clean: 
	rm $(lobjs) main


