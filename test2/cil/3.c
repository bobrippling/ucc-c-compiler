// RUN: %ucc %s -o %t && %t

int x = 5;
int f() {
	int x = 3;
	{
		extern int x;
		return x;
	}
}

main()
{
	return f() == 5;
}
