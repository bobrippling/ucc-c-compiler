OBJ = strbuf_fixed.o
LIB = strbuf.a

${LIB}: ${OBJ}
	ar rc $@ ${OBJ}
