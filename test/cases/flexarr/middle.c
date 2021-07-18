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

struct ContNested
{
	int yo;
	struct Flex f; // CHECK: warning: embedded flexible-array as nested in struct
};
