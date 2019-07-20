// RUN: %jmpcheck %s

void gotos(int cond)
{
	goto b;
a:
	goto c;
b:
	goto a;
c:
	goto b;
}

void infinite_loop_simple(int cond)
{
a:
	goto a;
}

void infinite_loop_complex()
{
	goto a;
c: goto a;
b: goto c;
a: goto b;
}

void infinite_loop_complex_straightline()
{
a:
b:
c:
	goto d;
d:
	goto e;
e:
	goto a;
}

int g(), a(), b();

void infinite_loop_with_code()
{
	if(g())
		a();
	else
		b();

	goto a;

c: goto a;
b: goto c;
a: goto b;
}
