// RUN: %ucc -S -o %t %s -O0 '-DG=g();' -DPRE=
// RUN: grep 'call.*g' %t
// RUN: %ucc -o %t %s -O0 '-DG=g(){}' -DPRE=inline
// RUN: ! grep 'call.*g' %t

G

// because this isn't inline, f() will be emitted, causing a linker error
PRE void f();

inline void f()
{
	g();
}

main()
{
	return 0;
}
