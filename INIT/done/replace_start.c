// FIXME: overrides the first, and so breaks the rest which depend on it
// FIXME: test middle array-range overrides

const int x[] = {
	[0 ... 100] = 5,
	[0] = 1
};
