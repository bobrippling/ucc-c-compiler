// RUN: %check %s
struct Flex
{
	int n;
	int vals[];
};

struct Cont
{
	struct Flex f; // CHECK: /warning: embedded struct with flex-array not final member/
	int a, b, c;
};

struct Fine
{
	int yo;
	struct Flex f; // CHECK: !/warn/
};
