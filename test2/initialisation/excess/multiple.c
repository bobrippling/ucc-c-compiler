// RUN: %check %s
struct A { int i, j; };

struct A x[2] = {
	1, 2, 3, 4 // CHECK: !/warn/
};

int y[2][2] = {
	1, 2, 3, 4 // CHECK: !/warn/
};

struct A x_[2] = {
	1, 2, 3, 4, 5 // CHECK: warning: excess initialiser for 'struct A [2]'
};

struct A x_2[2] = {
	1, 2, 3, 4, 5, 6 // CHECK: warning: excess initialiser for 'struct A [2]'
};

int y_[2][2] = {
	1, 2, 3, 4, 5 // CHECK: warning: excess initialiser for 'int ([2])[2]'
};

int y_2[2][2] = {
	1, 2, 3, 4, 5, 6 // CHECK: warning: excess initialiser for 'int ([2])[2]'
};

int y_fine[][2] = {
	1, 2, 3, 4, 5 // CHECK: !/warn/
	// [3][2]
};

struct A desig_st[2] = {
	[0] = 1, 2, 3, // CHECK: !/warn/
	//          ^ not excess, it's doing [1].i

	[1] = { 1, 2, 3 } // CHECK: warning: excess initialiser for 'struct A'
};

struct A desig_st2[2] = {
	[1] = 1, 2, 3 // CHECK: warning: excess initialiser for 'struct A [2]'
	//          ^ excess
};
