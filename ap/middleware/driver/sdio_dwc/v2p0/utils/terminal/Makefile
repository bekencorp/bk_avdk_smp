CC = gcc
core = ../../core
drvc = ../../drv/core
os = ../../os/no_os
build = ../../build
config = ../../config
core = ../../core
hal = ../../platform
test = ../../test/core
inc = -I../../inc -I. -I$(build) -I$(drvc) -I$(config) -I$(os) -I$(core) -I$(test) -I$(hal) -fPIC -Wall
objs = terminal.o stdcmd.o clink_terminal_server.o stdio_term.o
lobjs = $(build)/terminal.o 

all: $(objs)

terminal.o: terminal.c
	$(CC) $(inc) -c terminal.c -o $(build)/terminal.o

stdcmd.o: stdcmd.c
	$(CC) $(inc) -c stdcmd.c -o $(build)/stdcmd.o

clink_terminal_server.o: clink_terminal_server.c
	$(CC) $(inc) -c clink_terminal_server.c -o $(build)/clink_terminal_server.o

stdio_term.o: stdio_term.c
	$(CC) $(inc) -c stdio_term.c -o $(build)/stdio_term.o

clean: 
	rm $(lobjs)


