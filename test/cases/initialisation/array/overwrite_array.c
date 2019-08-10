// RUN: %layout_check %s
int ent1[10] = {
	[2] = 1,
	[3] = 2,
	[2] = 5
};
