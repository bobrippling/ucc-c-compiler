#ifdef DECL
typedef int a;
#else
a();
int *x;
#endif

// FIX - move symtab parenting to parse stage

main()
{
	a(*x);
}
