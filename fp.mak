fp_call: fp_funcall.o fp_lib.o

fp_funcall.o: fp_funcall.c
	./ucc -c -o $@ $<

fp_lib.o: fp_lib.c
	cc -c -o $@ $<
