// RUN: %check -e %s
union U
{
	int x;
	int y[]; // CHECK: /error: flexible array in a union/
};
