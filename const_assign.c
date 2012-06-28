#define __unused __attribute__((unused))
#define __unused2

func(       int i __unused) { }
kfunc(const int i __unused) { }

func_p(       char *i __unused) { }
kfunc_p(const char *i __unused) { }

main()
{
	char *a __unused2= "hi"; // WARN
	char *b __unused = (const char *)0; // WARN
	char  c __unused2= 'a';

	const char *d __unused2= "yo";
	const char *e __unused = (char *)0;
	const char  f __unused = 2;

	func(c);
	kfunc(c);

	func_p(a);
	kfunc_p(a);

	func_p(d); // WARN
	kfunc_p(d);
}
