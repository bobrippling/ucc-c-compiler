// RUN: %ucc -c %s
struct A
{
	int x, y;
};

_Static_assert((sizeof(struct A){ 1, 2 }) == sizeof(struct A), "buh?");
