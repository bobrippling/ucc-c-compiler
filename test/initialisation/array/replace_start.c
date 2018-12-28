// RUN: %layout_check %s
const char ent1[] = {
	[0 ... 8] = 5,
	[0] = 1
};
