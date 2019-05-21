struct Basic
{
	int x : 4, y : 4;
} bas = { 1, 2 };

_Static_assert(sizeof(bas) == sizeof(int), "");

struct Basic2
{
	char x : 4, y : 4;
} bas2 = { 1, 2 };

_Static_assert(sizeof(bas2) == sizeof(char), "");

int main()
{
	printf("bas=%p\n", &bas);
	printf("bas2=%p\n", &bas2);
	printf("diff = %zd\n", (char*)&bas2 - (char*)&bas);
}
