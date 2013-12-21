// RUN: %ucc -fsyntax-only %s

#define OP /
#define TY_CHK(exp, ty) (void)_Generic((exp), ty: 0)

char div_char(void)
{
	char a = 4, b = 2;
	TY_CHK(a OP b, int);
}

short div_short(void)
{
	short a = 4, b = 2;
	TY_CHK(a OP b, int);
}

int div_int(void)
{
	int a = 4, b = 2;
	TY_CHK(a OP b, int);
}

long div_long(void)
{
	long a = 4, b = 2;
	TY_CHK(a OP b, long);
}

main(void)
{
 div_char();
 div_short();
 div_int();
 div_long();
}
