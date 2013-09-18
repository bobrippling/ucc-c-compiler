INC = -I../cc1/out/ \
      -I../cc1/     \
      -I../util/

bb: bb.o
	cc -o $@ $< \
		../cc1/out/out.o \
		../cc1/funcargs.o \
		../cc1/type_ref.o \
		../cc1/decl.o

%.o: %.c
	cc ${INC} -o $@ $<
