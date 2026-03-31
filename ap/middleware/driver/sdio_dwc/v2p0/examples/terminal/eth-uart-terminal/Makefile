core = ../../../core
os = ../../../os/no_os
linux = ../../../os/linux
build = ../../../build
config = ../../../config
terminal = ../../../utils/terminal
inc = -I../../../inc -I. -I$(build) -I$(config) -I$(os) -I$(terminal) -I$(core)
objs = eth_term_stub.o uart_term_stub.o main.o
lobjs = $(build)/terminal.o $(build)/stdcmd.o $(build)/eth_term_stub.o $(build)/uart_term_stub.o $(build)/main.o $(build)/ctask.o $(build)/csem.o $(build)/init.o $(build)/ctask_no_os.o $(build)/ctask_sem.o $(build)/cmsg_queue.o $(build)/clog.o $(build)/fs_linux.o $(build)/fs.o $(build)/cwatch.o $(build)/clkcore.o $(build)/hal.o $(build)/regdesc.o $(build)/clink.o

main: $(objs)
	make -C $(os)
	make -C $(linux)
	make -C $(core)
	make -C $(terminal)
	cc $(inc) $(lobjs) -o main

main.o: main.c
	cc $(inc) -c main.c -o $(build)/main.o

eth_term_stub.o: eth_term_stub.c
	cc $(inc) -c eth_term_stub.c -o $(build)/eth_term_stub.o

uart_term_stub.o: uart_term_stub.c
	cc $(inc) -c uart_term_stub.c -o $(build)/uart_term_stub.o

clean: 
	rm $(lobjs) main


