OBJ = strbuf_fixed.o
LIB = strbuf.a

${LIB}: ${OBJ}
	ar rc $@ ${OBJ}

.%.d: %.c
	${CC} -MM ${CFLAGS} $< > $@

clean:
	rm -f ${OBJ} ${LIB} ${OBJ:%.o=.%.d}

-include ${OBJ:%.o=.%.d}

.PHONY: clean
