bf: bf.o bf_lib.o

bf.o: bf.c bf.h src/cc1/cc1
	./ucc -c -o $@ $<

bf_lib.o: bf_lib.c bf.h
	cc -c -o $@ $<
