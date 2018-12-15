// RUN: %ucc -fsyntax-only %s

#define TY_CHK(exp, ty)\
_Static_assert(        \
		1 == _Generic((exp), ty: 1),\
		"not " #ty)

TY_CHK(-0x1, int);
TY_CHK(0x0 - 1, int);
