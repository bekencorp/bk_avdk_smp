core = ../../../core
os = ../../../os/no_os
linux = ../../../os/linux
build = ../../../build
config = ../../../config
terminal = ../../../utils/terminal
test = ../../../test/core
clock = ../../../drv/clock
inc = -I../../../inc -I. -I$(build) -I$(config) -I$(os) -I$(terminal) -I$(core) -I$(test)
objs = clink_terminal.o
lobjs = $(build)/terminal.o $(build)/stdcmd.o $(build)/stdio_term.o $(build)/clink_terminal.o $(build)/ctask.o $(build)/csem.o $(build)/init.o $(build)/ctask_no_os.o $(build)/ctask_sem.o $(build)/cmsg_queue.o $(build)/clog.o $(build)/fs_linux.o $(build)/fs.o $(build)/cwatch.o $(build)/clink.o $(build)/ctask_linux.o $(build)/ctask_linux_sem.o $(build)/cee.o $(build)/hal.o $(build)/regdesc.o $(build)/clkcore.o $(build)/mmcm_clk.o

clink_terminal: $(objs)
	make -C $(os)
	make -C $(linux)
	make -C $(core)
	make -C $(test)
	make -C $(terminal)
	cc $(inc) $(lobjs) -o clink_terminal -I/usr/lib -I/usr/include -lpthread

clink_terminal.o: clink_terminal.c
	cc $(inc) -c clink_terminal.c -o $(build)/clink_terminal.o -I/usr/lib -I/usr/include
regdesc.o: regdesc.c
	$(CC) $(inc) -c regdesc.c -o $(build)/regdesc.o
clean:
	rm $(lobjs) clink_terminal


