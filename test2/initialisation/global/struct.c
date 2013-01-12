// RUN: [ `%ucc %s -S -o- | grep '[123]' | wc -l` -eq 3 ]

struct
{
	int i, j, k;
} a[] = { 1, 2, 3 };
