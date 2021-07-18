// RUN: %ucc -o %t %s
// RUN: %t; [ $? -eq 7 ]

long long l = 2;

main()
{
	l += 5LL;

	return l;
}
