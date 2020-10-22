// RUN: %ucc -o %t %s
// RUN: %t; [ $? -eq 10 ]
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
