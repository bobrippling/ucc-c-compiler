#CFLAGS += -std=c99
CC = ../../ucc

test: eg
	./eg
	./eg | grep '^4$$'

eg: apply.o eg.o

clean:
	rm -f *.o eg
