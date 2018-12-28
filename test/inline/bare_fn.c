// RUN: %ucc -o %t %s -O0 -DG= -DPRE=; [ $? -ne 0 ]
// RUN: %ucc -o %t %s -O0 '-DG=g(){}' -DPRE=inline

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
