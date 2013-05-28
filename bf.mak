bf: bf_wrap.o bf.o

bf.o: bf.c bf.h src/cc1/cc1
	./ucc -c -o $@ $<

bf_wrap.o: bf_wrap.c bf.h
	cc -c -o $@ $<
