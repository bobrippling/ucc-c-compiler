main()
{
#define decl(sp, val) typeof(val) sp = val;
	decl(a, 5);
	typeof(struct A *) b;
	return a;
}
