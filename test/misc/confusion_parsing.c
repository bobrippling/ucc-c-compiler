// RUN: %ucc -DDECL -o %t %s && %t
// RUN: %ucc        -o %t %s && %t

#ifdef DECL
typedef int a;
#else
a(){}

int i;
int *x = &i;
#endif

// FIX - move symtab parenting to parse stage

main()
{
	a(*x);

	return 0;
}
