// RUN: [ `%ucc -S -o- %s | grep -c cvtss2sd` -eq 2 ]
// exactly two promotions

vari(float f, double d, ...);
noproto();

g(float f, double d)
{
	vari(f /* not promoted */, d, f /* promoted */, d);

	noproto(f /* promoted */, d);
}
