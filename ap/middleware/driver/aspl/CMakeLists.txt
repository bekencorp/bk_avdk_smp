
list(APPEND incs aspl)
list(APPEND srcs
	aspl/sspl_lock.c
	aspl/sspl_lock_test.c
	aspl/szymanski_lock/src/szymanski_lock.c
	aspl/aspl_lock.c
)

set(incs ${incs} PARENT_SCOPE)
set(srcs ${srcs} PARENT_SCOPE)

