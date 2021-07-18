CC = ../../ucc
CFLAGS = -g
OBJ = rand.o randstack.o

randstack.so: ${OBJ}
	${CC} -shared -o $@ $^

demo: demo.o randstack.so
	${CC} -o $@ $^

run_demo: demo
	LD_LIBRARY_PATH=. ./demo

clean:
	rm -f randstack.so rand.o randstack.o demo demo.o

.PHONY: run_demo clean
