all:
	make -C src
	make -C lib

clean:
	make -C src clean
	make -C lib clean

.PHONY: all clean
