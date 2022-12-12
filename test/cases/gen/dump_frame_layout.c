// RUN: %ucc -S -o- -fdump-frame-layout %s 2>&1 | grep '^frame' | %stdoutcheck %s

void g(void *);

void f(int i, int j){
	// STDOUT: frame: -16(%rbp): i (argument)
	// STDOUT: frame:  -8(%rbp): j (argument)

	char buf[i][j]; // STDOUT: frame: 16  -  48: buf (variably-modified)
	int q = i + 3; // STDOUT: frame: 48  -  52: q
	char *p = "hi"; // STDOUT: frame: 56  -  64: p
	struct A {
		char x;
		char *p;
	} a = { .p = p, .x = 'a' }; // STDOUT: frame: 64  -  80: a

	g(buf);
}
