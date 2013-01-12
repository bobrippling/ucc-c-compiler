// RUN: %ucc -o %t %s
// RUN: %t

struct A
{
	char c;
	struct B
	{
		void *p;
		int j, k;
	} b;
};

main()
{
	return sizeof(struct A) == 24 && sizeof(struct B) == 16 ? 0 : 1;
}
