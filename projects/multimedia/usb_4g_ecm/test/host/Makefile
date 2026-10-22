# Host-side check of modem_at.c's byte-stream handling. Runs on the build
# machine with the native compiler - no target, no board, not part of the
# firmware build. See test_modem_at.c for what it covers.
#
#   make -C test/host run
#
# The stubs under stub/ stand in for armino's os layer and log macros, so the
# file under test is the project's real modem_at.c and not a copy.

AP_DIR := ../../ap
CFLAGS := -std=gnu11 -Wall -Wextra -Wno-unused-parameter -Istub -I$(AP_DIR)

test_modem_at: test_modem_at.c $(AP_DIR)/modem_at.c $(AP_DIR)/modem_at.h stub/os/os.h
	$(CC) $(CFLAGS) -o $@ test_modem_at.c $(AP_DIR)/modem_at.c

run: test_modem_at
	./test_modem_at

clean:
	rm -f test_modem_at test_modem_at_threads

.PHONY: run clean

# Real mutexes/semaphores and an independent RX thread, unlike the parser fixture.
test_modem_at_threads: test_modem_at_threads.c $(AP_DIR)/modem_at.c stub/os/threaded_os.h
	$(CC) $(CFLAGS) -DMODEM_AT_THREADED_TEST -pthread -o $@ test_modem_at_threads.c $(AP_DIR)/modem_at.c

run-threads: test_modem_at_threads
	./test_modem_at_threads

.PHONY: run-threads
