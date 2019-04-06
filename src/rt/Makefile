include ../config.mk

CFLAGS += -fno-sanitize=all

all: dsohandle.o

clean:
	@echo clean rt
	$Qrm -f dsohandle.o

.PHONY: clean all
include ../Makefile.common
