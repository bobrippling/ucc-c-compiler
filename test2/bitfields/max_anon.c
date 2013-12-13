// RUN: %check -e %s

struct
{
	int i;
	int : 99; // CHECK: /error: bitfield too large/
};
