include ../config.mk

WARNINGS = -Wall -Wextra -pedantic -Werror=incompatible-pointer-types -Werror=implicit-function-declaration -Wmissing-prototypes -Wstrict-prototypes

CC = ../ucc/ucc
CFLAGS = -std=c11 ${WARNINGS}

all: dsohandle.o

clean:
	@echo clean rt
	$Qrm -f dsohandle.o

.PHONY: clean all
include ../Makefile.common
