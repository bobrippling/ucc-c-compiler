// RUN: [ `./ucc -Xprint default_promotions.c | grep -A1 'expr: cast' | grep tree_type | grep ': double'` -eq 2 ]
// exactly two promotions

vari(float f, double d, ...);
noproto();

g(float f, double d)
{
	vari(f /* not promoted */, d, f /* promoted */, d);

	noproto(f /* promoted */, d);
}
