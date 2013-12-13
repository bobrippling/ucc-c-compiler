// RUN: %ocheck 15 %s

f(int i0,
	int i1,
	int i2,
	int i3,
	int i4,
	int i5,
	int i6,
	int i7,
	int i8,
	int i9)
{
	return i0 + i1 - i2 + i3 - i4 + i5 - i6 + i7 - i8 + i9;
}

main()
{
	return f(5, 6, 7, 8, 9, 10, 11, 12, 13, 14);
}
