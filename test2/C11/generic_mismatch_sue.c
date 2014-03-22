// RUN: %check -e %s

main()
{
	return _Generic(
				*(__typeof((*(struct A { int (*i)(union f); } *)0).i))0 // CHECK: /warning:.*only visible inside/
				, char: 5, int (*)(union f f): 1); // CHECK: /warning:.*only visible inside/
}
