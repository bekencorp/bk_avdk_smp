include Makefile.inc

DIRS= ${LIB} ${CA} 

all:
	for i in ${DIRS}; do \
		$(MAKE) -C $$i || exit 1; \
	done

clean:
	for i in ${DIRS}; do \
		$(MAKE) -C $$i clean || exit 1; \
	done
	
install:
	cp -f ${CA}/wfa_ca /valid/bin;
	cp -f dini/dini_init.sh /valid;
	cp -f dini/sigma_dut.sh /valid;
	cp -f dini/bridge_init.sh /valid;
	cp -f dini/bridge_setup.sh /valid;
	cp -f dini/dut.conf /valid;