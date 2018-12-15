PWD = $(shell pwd)
CONFIGURE_OUTPUT = src/config.custom.mk

.PHONY: bootstrap clean-bootstrap clean-stage1 clean-stage2 clean-stage3

bootstrap: dsohandle.o stage3

clean-bootstrap:
	rm -rf bootstrap

clean-stage1:
	rm -rf bootstrap/stage1
clean-stage2:
	rm -rf bootstrap/stage2
clean-stage3:
	rm -rf bootstrap/stage3

bootstrap/stage1/${CONFIGURE_OUTPUT}: tools/link_r
	mkdir -p bootstrap/stage1
	cd bootstrap/stage1 && ../../configure
stage1: bootstrap/stage1/${CONFIGURE_OUTPUT}
	make -Cbootstrap/stage1/src

bootstrap/stage2/${CONFIGURE_OUTPUT}:
	mkdir -p bootstrap/stage2
	cd bootstrap/stage2 && ../../configure CC=${PWD}/bootstrap/stage1/src/ucc/ucc\ -fuse-cpp=${PWD}/tools/syscpp
stage2: stage1 bootstrap/stage2/${CONFIGURE_OUTPUT}
	make -Cbootstrap/stage2/src

bootstrap/stage3/${CONFIGURE_OUTPUT}:
	mkdir -p bootstrap/stage3
	cd bootstrap/stage3 && ../../configure CC=${PWD}/bootstrap/stage2/src/ucc/ucc\ -fuse-cpp=${PWD}/tools/syscpp
stage3: stage2 bootstrap/stage3/${CONFIGURE_OUTPUT}
	make -Cbootstrap/stage3/src

tools/link_r: tools/link_r.c
	${CC} -o $@ $<
