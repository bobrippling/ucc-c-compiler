#define __unused __attribute__((unused))
#define __unused2

void func(       int i __unused) { }
void kfunc(const int i __unused) { }

void func_p(       char *i __unused) { }
void kfunc_p(const char *i __unused) { }

void main()
{
	char *a __unused2;
	char *b __unused;
	char  c __unused2;

	const char *d __unused2;
	const char *e __unused;
	const char  f __unused;

	a = "hi"; // WARN
	b = (const char *)0; // WARN
	c = 'a';

	d = "yo";
	e = (char *)0;
	//f = 2;

	func(c);
	kfunc(c);

	func_p(a);
	kfunc_p(a);

	func_p(d); // WARN
	kfunc_p(d);

	const int i = 2;
	int j __unused;

	j = i;
	func(i);
}
