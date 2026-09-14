include ../Makefile.inc

PROGS = wfa_ca 

all: ${PROGS}

wfa_ca: wfa_ca.o
	${CC} ${CFLAGS} -DDEBUG=1 -o $@ wfa_ca.o ${CADUTLIBS}

clean:
	rm -f ${PROGS} ${CLEANFILES}