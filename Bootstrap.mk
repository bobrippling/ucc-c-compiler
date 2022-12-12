PWD = $(shell pwd)
CONFIGURE_OUTPUT = src/config.custom.mk

CPPFLAGS_BOOTSTRAP = \
		 -fuse-cpp=${PWD}/tools/syscpp

# -Wno-return-undef: reachability depends on libc's _Noreturn/__GNUC__
CFLAGS_BOOTSTRAP = \
		 -fshow-warning-option \
		 -Wno-return-undef

# turn off debugging for bootstrap comparison
CFLAGS_BOOTSTRAP += -g0

CC_STAGE1 = ${PWD}/src/ucc/ucc
CC_STAGE2 = ${PWD}/bootstrap/stage2/src/ucc/ucc
CC_STAGE3 = ${PWD}/bootstrap/stage3/src/ucc/ucc

.PHONY: bootstrap clean-bootstrap clean-stage1 clean-stage2 clean-stage3

bootstrap: stage3

bootstrap-compare: bootstrap compare

compare: stage3
	cmp ${CC_STAGE2} ${CC_STAGE3}
	cmp ${PWD}/bootstrap/stage[23]/src/cpp2/cpp
	cmp ${PWD}/bootstrap/stage[23]/src/cc1/cc1

compare-obj: stage3
	tools/bootstrap-compare-obj ${PWD}/bootstrap/stage[23]/src/cpp2
	tools/bootstrap-compare-obj ${PWD}/bootstrap/stage[23]/src/cc1
	tools/bootstrap-compare-obj ${PWD}/bootstrap/stage[23]/src/ucc

clean-bootstrap:
	rm -rf bootstrap

clean-stage1: clean

clean-stage2:
	rm -rf bootstrap/stage2
clean-stage3:
	rm -rf bootstrap/stage3

stage1: src

bootstrap/stage2/${CONFIGURE_OUTPUT}: tools/link_r
	mkdir -p bootstrap/stage2
	cd bootstrap/stage2 && ../../configure CC=${CC_STAGE1} 'CFLAGS=${CFLAGS_BOOTSTRAP}' 'CPPFLAGS=${CPPFLAGS_BOOTSTRAP}'
stage2: stage1 bootstrap/stage2/${CONFIGURE_OUTPUT}
	make -Cbootstrap/stage2/src

bootstrap/stage3/${CONFIGURE_OUTPUT}:
	mkdir -p bootstrap/stage3
	cd bootstrap/stage3 && ../../configure CC=${CC_STAGE2} 'CFLAGS=${CFLAGS_BOOTSTRAP}' 'CPPFLAGS=${CPPFLAGS_BOOTSTRAP}'
stage3: stage2 bootstrap/stage3/${CONFIGURE_OUTPUT}
	make -Cbootstrap/stage3/src

tools/link_r: tools/link_r.c stage1
	${CC_STAGE1} -o $@ $< ${CFLAGS_BOOTSTRAP} ${CPPFLAGS_BOOTSTRAP}

check-bootstrap: bootstrap-compare
	cd test && ./run_tests -i ignores -j4 '--ucc=${CC_STAGE3}' cases
