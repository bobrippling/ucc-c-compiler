// RUN: %check %s

struct A
{
	short x : 3;
	int z : 7; // CHECK: warning: bitfield 'int' type doesn't match its packed bitfield type of 'short'
};
