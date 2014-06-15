// RUN: %jmpcheck %s

f(int cond)
{
	goto b;
a:
	goto c;
b:
	goto a;
c:
	goto b;
}
