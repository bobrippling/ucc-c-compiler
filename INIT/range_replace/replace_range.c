struct A { int i, j; } a[] = {
	[0 ... 5] = { 1, 2 },
	[0].j = 5,
};
