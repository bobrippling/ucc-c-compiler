CC = ../../src/cc

a.out: tim.o stat.o
	ld tim.o stat.o ../../lib/*.o ../../lib/sys/*.o

tim.o: tim.c
	${CC} -c $<

stat.o: stat.c
	${CC} -c $<

info:
	@cc -o info info.c
	@./info
	@rm -f info

clean:
	rm -f *.o a.out core

stat.o: stat.c stat.h
tim.o: tim.c stat.h
