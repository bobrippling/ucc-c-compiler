UCC = ../../ucc

all: clang gcc ucc

a:
	echo $@ > $@
b:
	echo $@ > $@
c:
	echo $@ > $@

setup: a b c

clang: setup
	clang -E a
	clang -c a
	-clang a

gcc: setup
	gcc -E a
	gcc -c a
	-gcc a

ucc: setup
	${UCC} -E a
	${UCC} -c a
	-${UCC} a

clean:
	rm -f a b c
