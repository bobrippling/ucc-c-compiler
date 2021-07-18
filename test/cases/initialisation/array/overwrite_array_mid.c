// RUN: %layout_check %s
int ent1[] = {
	[0 ... 10] = 3,
	[5] = 2
};
