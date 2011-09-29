CFLAGS = -Wall -Wextra -pedantic

cpp: pp.o main.o utils.o
	${CC} ${CFLAGS} -o $@ $^

clean:
	rm -f *.o cpp

.PHONY: clean
