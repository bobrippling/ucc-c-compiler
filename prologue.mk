UCC = ./ucc

prologue: prologue.o prologue_support.o
	${UCC} -o $@ $^

prologue.o: prologue.c src/cc1/cc1
	${UCC} -c -o $@ $<

prologue_support.o: prologue.c
	${CC} -c -o $@ $< -DIMPL
