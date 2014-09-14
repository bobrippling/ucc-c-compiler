// RUN: %check -e %s

int x[] = { // CHECK: error: global brace initialiser not constant
	[0 ... 1] = f()
};
