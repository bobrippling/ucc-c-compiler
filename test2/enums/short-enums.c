// RUN: %ucc -fsyntax-only %s -fshort-enums

#define E(nam, v) enum nam { nam = v }

E(minus, -600);

E(char_m1, 255);
E(char_,   256);
E(char_p1, 257);

E(short_m1, 65535);
E(short_,   65536);
E(short_p1, 65537);

E(int_m1, 4294967295);
E(int_,   4294967296);
E(int_p1, 4294967297);

#ifndef __UCC__
#warning old static assert
#define _Static_assert(x,s) {typedef char y[(x)?1:-1];}
#endif

int main()
{
/*printf("sizeof(" #t ") = %d\n", sizeof(t))*/
#define sz(exp, t) \
_Static_assert(sizeof(t) == exp, "sizeof("#t") not "#exp)

sz(2, enum minus);
sz(1, enum char_m1);
sz(2, enum char_);
sz(2, enum char_p1);
sz(2, enum short_m1);
sz(4, enum short_);
sz(4, enum short_p1);
sz(4, enum int_m1);
sz(8, enum int_);
sz(8, enum int_p1);
}
