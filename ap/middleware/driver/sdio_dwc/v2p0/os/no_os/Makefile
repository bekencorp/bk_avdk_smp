CC = gcc
core = ../../core
build = ../../build
config = ../../config
terminal = ../../utils/terminal
test = ../../test/core
drvc = ../../drv/core
inc = -I../../inc -I. -I$(build) -I$(drvc) -I$(config) -I$(core) -I$(terminal) -I$(test) -g -fPIC -Wall
objs = ctask_no_os.o ctask_sem.o
robjs = $(build)/ctask_no_os.o $(build)/ctask_sem.o

all: $(objs)

ctask_no_os.o: ctask_no_os.c
	$(CC) $(inc) -c ctask_no_os.c -o $(build)/ctask_no_os.o
ctask_sem.o: ctask_sem.c
	$(CC) $(inc) -c ctask_sem.c -o $(build)/ctask_sem.o

clean: 
	rm $(robjs)


