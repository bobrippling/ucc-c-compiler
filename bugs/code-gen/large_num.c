// compile with -mstackrealign

int printf();

int f(int i)
{
	return i + 3;
}

int main()
{
  printf("%d\n", f(5));
}
