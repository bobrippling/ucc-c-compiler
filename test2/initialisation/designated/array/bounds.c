// RUN: ! %ucc %s

char x[3] = {
	[5] = 2
};
