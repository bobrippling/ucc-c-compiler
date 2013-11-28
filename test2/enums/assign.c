// RUN: %check %s

enum E
{
	A = 1 << 0,
	B = 1 << 1,
	C = 1 << 2,
};

enum E a = ~0UL; // CHECK: /warning: .*truncate/
enum E b = ~0;   // CHECK: !/warn/
enum E c = ~0U;  // CHECK: !/warn/
