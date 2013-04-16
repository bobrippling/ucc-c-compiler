// RUN: %asmcheck %s

char div_char(void)
{
	char a = 4, b = 2;
	return a OP b;
}

short div_short(void)
{
	short a = 4, b = 2;
	return a OP b;
}

int div_int(void)
{
	int a = 4, b = 2;
	return a OP b;
}

long div_long(void)
{
	long a = 4, b = 2;
	return a OP b;
}

main(void)
{
 div_char();
 div_short();
 div_int();
 div_long();
}
