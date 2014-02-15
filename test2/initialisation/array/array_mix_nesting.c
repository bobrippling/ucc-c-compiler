// RUN: %layout_check %s
int ent1[][2] = {
	{1},
	2, 3,
	{4},
	5, {6},
	{7}
};
