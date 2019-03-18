// RUN: %ocheck 0 %s

// bit width = 2 -> int
// bit width = 4 -> long
// bit width = 8 -> 2x long
enum
{
	W = 8
};

struct A
{
	int a : W;
	int b : W;
	int c : W;
	int d : W;
	int e : W;
	int f : W;
	int g : W;
	int h : W;
	int i : W;
	int j : W;
	int k : W;
	int l : W;
	int m : W;
	int n : W;
	int o : W;
	int p : W;
	int q : W;
	int r : W;
	int s : W;
	int t : W;
	int u : W;
	int v : W;
	int w : W;
	int x : W;
	int y : W;
	int z : W;
// perl -e 'print "\tint " . chr(97 + $_) . " : 1;\n" for(1 ... 26)'
};

struct B
{
	unsigned a : 1, b : 2, c : 3;
};

struct A f()
{
	return (struct A){
		1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26
	};
}

struct B g()
{
	return (struct B){
		0, 2, 5
	};
}

main()
{
	struct A a = f();

	if(a.a != 1
	|| a.b != 2
	|| a.c != 3
	|| a.d != 4
	|| a.e != 5
	|| a.f != 6
	|| a.g != 7
	|| a.h != 8
	|| a.i != 9
	|| a.j != 10
	|| a.k != 11
	|| a.l != 12
	|| a.m != 13
	|| a.n != 14
	|| a.o != 15
	|| a.p != 16
	|| a.q != 17
	|| a.r != 18
	|| a.s != 19
	|| a.t != 20
	|| a.u != 21
	|| a.v != 22
	|| a.w != 23
	|| a.x != 24
	|| a.y != 25
	|| a.z != 26)
	{
		abort();
	}

	struct B b = g();
	if(b.a != 0
	|| b.b != 2
	|| b.c != 5)
	{
		abort();
	}

	return 0;
}
