// RUN: %ucc -fsyntax-only %s

void vla(int [*]);
void vla(int [const *]);

f(int *p)
{
	int vla2[*(int *)p];
	int vla[*];
}

g(int len, char data[len][len])
{
}
