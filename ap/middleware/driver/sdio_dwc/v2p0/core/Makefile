CC = gcc
os = ../os/csim
core = ../core
build = ../build
config = ../config
terminal = ../utils/terminal
test = ../test/core
clk = ../drv/clock
drvc = ../drv/core
plat = ../platform
inc = -I../inc -I. -I$(build) -I$(config) -I$(drvc) -I$(os) -I$(core) -I$(clk) -I$(plat) -I$(terminal) -I$(test) -g -fPIC -Wall
objs = init.o ctask.o csem.o regdesc.o cmsg_queue.o fs.o clink.o clog.o cwatch.o delay.o
robjs = $(build)/init.o $(build)/ctask.o $(build)/csem.o $(build)/cmsg_queue.o $(build)/clink.o $(build)/fs.o $(build)/clog.o $(build)/cwatch.o $(build)/regdesc.o $(build)/delay.o

all: $(objs)
init.o: init.c
	$(CC) $(inc) -c init.c -o $(build)/init.o
fs.o: fs.c
	$(CC) $(inc) -c fs.c -o $(build)/fs.o
clog.o: clog.c
	$(CC) $(inc) -c clog.c -o $(build)/clog.o
clink.o: clink.c
	$(CC) $(inc) -c clink.c -o $(build)/clink.o
ctask.o: ctask.c
	$(CC) $(inc) -c ctask.c -o $(build)/ctask.o
csem.o: csem.c
	$(CC) $(inc) -c csem.c -o $(build)/csem.o
regdesc.o: regdesc.c
	$(CC) $(inc) -c regdesc.c -o $(build)/regdesc.o
cmsg_queue.o: cmsg_queue.c
	$(CC) $(inc) -c cmsg_queue.c -o $(build)/cmsg_queue.o
cwatch.o: cwatch.c
	$(CC) $(inc) -c cwatch.c -o $(build)/cwatch.o
delay.o: delay.c
	$(CC) $(inc) -c delay.c -o $(build)/delay.o
clean: 
	rm $(robjs)

