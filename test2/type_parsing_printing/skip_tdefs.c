// RUN: %check %s

typedef struct A
{
	int i;
} tdef_a;

typedef tdef_a tdef_b;
typedef tdef_b volatile tdef_c;
typedef const tdef_c tdef_d;
typedef tdef_d __attribute((used)) tdef_e;
typedef tdef_e tdef_f;

g(int *p);

f()
{
	tdef_f local;

	g(&local); // CHECK: note: 'int *' vs 'tdef_f (aka 'struct A') volatile const *'
	// ensure we get "struct A" in the warning, not any typedefs
}
