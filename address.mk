CFLAGS = -g -fsanitize=address
LDFLAGS = -g -lasan -L/usr/local/Cellar/gcc/5.1.0/lib/gcc/5/

address: address.s
	./ucc -g -o $@ $< ${LDFLAGS}

address.s: address.c
	./ucc -S -o $@ $< ${CFLAGS}
