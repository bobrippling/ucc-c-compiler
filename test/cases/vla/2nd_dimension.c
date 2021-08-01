// RUN: %ucc -fsyntax-only %s

f(int x)
{
	int ar[2][x]; // tests type_is_variably_modified()
}
