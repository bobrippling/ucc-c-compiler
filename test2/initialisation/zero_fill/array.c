// RUN: [ `%ucc %s -S -o- | grep 'mov.*[01]' | wc -l` -eq 4 ]

f()
{
	int x[4] = { 1 };
}
