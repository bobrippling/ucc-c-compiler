struct Flex
{
	int n;
	int vals[];
};

struct Cont
{
	struct Flex f;
	int a, b, c;
};
