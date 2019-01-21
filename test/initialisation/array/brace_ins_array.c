// RUN: %layout_check %s
int ent1[][2] = {
	// insert here
	1,
	2,
	// finish
	// insert here
	3,
	4
	// finish
};
