// RUN: %check -e %s

struct A
{
	int i  : 32; // CHECK: !/(error|warn)/
	long l : 64; // CHECK: !/(error|warn)/

	int j  : 33; // CHECK: /error: bitfield too large/
};
