CC = ./ucc
OBJ = vari.o abi.o

vari: ${OBJ}
	cc -o $@ ${OBJ}

vari.o: vari.c src/cc1/cc1
abi.o: abi.c # src/cc1/cc1
	cc -c -o $@ $<
