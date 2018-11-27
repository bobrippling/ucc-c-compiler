PWD = $(shell pwd)

.PHONY: bootstrap bootstrap_clean

bootstrap: stage3

bootstrap_clean:
	rm -rf bootstrap

stage1 stage2 stage3: tools/link_r

bootstrap/stage1/config.mk:
	mkdir -p bootstrap/stage1
	cd bootstrap/stage1 && ../../configure
stage1: bootstrap/stage1/config.mk
	make -Cbootstrap/stage1/src

bootstrap/stage2/config.mk:
	mkdir -p bootstrap/stage2
	cd bootstrap/stage2 && ../../configure
stage2: stage1 bootstrap/stage2/config.mk
	make -Cbootstrap/stage2/src CC=${PWD}/bootstrap/stage1/src/ucc/ucc\ -fuse-cpp=${PWD}/tools/syscpp

bootstrap/stage3/config.mk:
	mkdir -p bootstrap/stage3
	cd bootstrap/stage3 && ../../configure
stage3: stage2 bootstrap/stage3/config.mk
	make -Cbootstrap/stage3/src CC=${PWD}/bootstrap/stage2/src/ucc/ucc\ -fuse-cpp=${PWD}/tools/syscpp

tools/link_r: tools/link_r.c
	${CC} -o $@ $<
