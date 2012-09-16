all: gcc ucc

#ucc_gcc gcc_ucc

gcc: pack_g.o pack2_g.o
	cc -o $@ $^

ucc: pack_u.o pack2_u.o
	../ucc -o $@ $^

pack_g.o: pack.c
	cc -c -o $@ $<
pack2_g.o: pack2.c
	cc -c -o $@ $<

pack_u.o: pack.c
	../ucc -c -o $@ $<
pack2_u.o: pack2.c
	../ucc -c -o $@ $<

clean:
	rm -f *.o gcc ucc
