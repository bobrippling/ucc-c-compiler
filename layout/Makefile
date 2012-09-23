a: a.o b.o
	cc -o $@ a.o b.o

a.o: a.c
	cc -c $<

b.o: b.s
	as -o $@ $<
