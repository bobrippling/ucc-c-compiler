PWD = $(shell pwd)

.PHONY: bootstrap bootstrap_clean

bootstrap: stage3

bootstrap_clean:
	rm -rf bootstrap

bootstrap/stage1/config.mk:
	mkdir -p bootstrap/stage1
	cd bootstrap/stage1 && ../../configure
stage1: bootstrap/stage1/config.mk
	cd bootstrap/stage1 && make -Csrc

bootstrap/stage2/config.mk:
	mkdir -p bootstrap/stage2
	cd bootstrap/stage2 && ../../configure
stage2: stage1 bootstrap/stage2/config.mk
	cd bootstrap/stage2 && make -Csrc CC=${PWD}/bootstrap/stage1/src/ucc/ucc\ -fsystem-cpp\ -U__GNUC__\ -D__UCC__

bootstrap/stage3/config.mk:
	mkdir -p bootstrap/stage3
	cd bootstrap/stage3 && ../../configure
stage3: stage2 bootstrap/stage3/config.mk
	cd bootstrap/stage3 && make -Csrc CC=${PWD}/bootstrap/stage2/src/ucc/ucc\ -fsystem-cpp\ -U__GNUC__\ -D__UCC__
