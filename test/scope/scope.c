// RUN: %ocheck 10 %s

typedef unsigned size_t;
int i = 2;

size_t sz = 3;

main()
{
	int i = 5;
	i = 3;
	int j = 2;
	return i + j + sz + ({ extern i; i; });
}
