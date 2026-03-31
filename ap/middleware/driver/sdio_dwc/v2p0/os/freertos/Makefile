CC = gcc
core = ../../core
build = ../../build
config = ../../config
terminal = ../../utils/terminal
test = ../../test/core
drvc = ../../drv/core
inc = -I../../inc -I. -I$(build) -I$(drvc) -I$(config) -I$(core) -I$(terminal) -I$(test) -g -fPIC -Wall
objs = fs_linux.o ctask_linux_sem.o ctask_linux.o
robjs = $(build)/fs_linux.o $(build)/ctask_linux_sem.o $(build)/ctask_linux.o

all: $(objs)

fs_linux.o: fs_linux.c
	$(CC) $(inc) -c fs_linux.c -o $(build)/fs_linux.o
ctask_linux.o: ctask_linux.c
	$(CC) $(inc) -c ctask_linux.c -o $(build)/ctask_linux.o
ctask_linux_sem.o: ctask_linux_sem.c
	$(CC) $(inc) -c ctask_linux_sem.c -o $(build)/ctask_linux_sem.o

clean: 
	rm $(robjs)


