UCC = ../../ucc

test: deps
	diff -u deps expected

deps:
	${UCC} -MM a.c >$@

clean:
	rm -f deps

.PHONY: test clean
