OBJ = vla_main.o vla_oob.o vla_sup.o
CC = ../../ucc

CFLAGS = -g -fsanitize=undefined -fno-trapv
LDFLAGS = -g

.SUFFIXES:
.SECONDARY:

vla: ${OBJ}
	${CC} ${LDFLAGS} -o $@ ${OBJ}

%.o: %.s
	${CC} -c -o $@ $<

%.s: %.c
	${CC} ${CFLAGS} -S -o $@ $<

.PHONY: clean
clean:
	rm -f ${OBJ} ${OBJ:.o=.s} vla
