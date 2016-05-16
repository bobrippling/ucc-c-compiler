typedef struct colour {
	int padding;
	struct { // index 1
		int a;
		int b;
		struct{ // index 2
			int c,d,e;
			struct{ // index 3
				struct{ // index 0
					int f, g, h, i, j; // index 4
				};
			};
		};
	};
	int after;
} colour;

colour x = { .j = 3 };

int *p = &x.j;

f(colour *p)
{
	return p->j;
}
// STDOUT: type $struct1_ = {i4, i4, i4, i4, i4}
// STDOUT: type $struct2_ = {$struct1_}
// STDOUT: type $struct3_ = {i4, i4, i4, $struct2_}
// STDOUT: type $struct4_ = {i4, i4, $struct3_}
// STDOUT: type $struct5_colour = {i4, $struct4_, i4}
// STDOUT: $x = $struct5_colour global { 0, { 0, 0, { 0, 0, 0, { { 0, 0, 0, 0, 3 } } } }, 0 }
// STDOUT: $p = i4* global $_x add 40 anyptr
// STDOUT: $f = i4($struct5_colour* $p)
// STDOUT: {
// STDOUT: $a_p_0 = alloca $struct5_colour*
// STDOUT: store $a_p_0, $p
// STDOUT: $0 = load $a_p_0
// STDOUT: $1 = elem $0, i4 1
// STDOUT: $2 = elem $1, i4 2
// STDOUT: $3 = elem $2, i4 3
// STDOUT: $4 = elem $3, i4 0
// STDOUT: $5 = elem $4, i4 4
// STDOUT: $6 = load $5
// STDOUT: ret $6
// STDOUT: }
