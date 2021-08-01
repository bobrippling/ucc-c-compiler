// RUN: %check %s
struct
{
	int x[2];
} a = {
	.x[0] = 5 // CHECK: !/warning:.*missing braces/
};
