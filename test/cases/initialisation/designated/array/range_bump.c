// RUN: %layout_check %s

int x[] = {
	[0 ... 1] = 0,
	// need to make sure the next is [2]
	500
};

int y[] = {
	[0 ... 1] = 0,
	500,
	[2] = 6
};
