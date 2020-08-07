.SECONDARY:

CPPFLAGS = -fuse-cpp=tools/syscpp
CFLAGS = -g -fdebug-compilation-dir=/tmp/ -w
GET_STAGE = $$(echo $@ | sed 's/.*\([23]\).*/\1/')
GET_CC = bootstrap/stage${GET_STAGE}/src/ucc/ucc

all: lno-2.s lno-3.s

cmp-%: lno-2.% lno-3.%
	suff=$$(echo $@ | sed 's/.*\(.\)/\1/'); cmp lno-2.$$suff lno-3.$$suff

lno-%.i: bootstrap/stage%/src/ucc/ucc.c
	(cd bootstrap/stage${GET_STAGE}/ && ./src/ucc/ucc -E src/ucc/ajdadaouhdaaaaaaaaaaaaaaaaaaaaaaaaaahhhhhhhhhhh ucc.c ${CPPFLAGS} >$@

lno-%.s: lno-%.i
	${GET_CC} -S $< ${CFLAGS} >$@

clean:
	rm -f lno-*.[si]
