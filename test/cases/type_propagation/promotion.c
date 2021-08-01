// RUN: %check --only -e %s -DERRORS
// RUN: %ucc -target x86_64-linux -S -o- %s -fshort-enums | %stdoutcheck %s

#ifdef ERRORS
void f(int, ...);

void g(void);

int main()
{
	// shouldn't try to cast `void` to anything:
	f(g()); // CHECK: error: argument 1 to f requires non-void expression
	// CHECK: ^error: mismatching types, argument 1 to f

	f(1, g()); // CHECK: error: argument 2 to f requires non-void expression
}

#else

enum E {
	A, B, C = 256,
};

enum E second(enum E *p) {
	// STDOUT: second:
	// STDOUT: movw 4(
	// STDOUT: movzwl
	return p[2];
}

void promote(int);

int main() {
	// STDOUT: main:

	enum E e = C;
	// STDOUT: movw $256,
	// STDOUT: movzwl
	// STDOUT: /call.*promote/
	promote(e); // -fshort-enums, we should promote here

	enum E es[] = {
		A, B, C,
	};
	// STDOUT: movw $256,

	// STDOUT: /call.*second/
	return second(es) == B ? 0 : 1;
}
#endif
