// RUN: %check -e %s

enum A
{
	X = sizeof(enum A) // CHECK: error: sizeof incomplete type enum A
};
