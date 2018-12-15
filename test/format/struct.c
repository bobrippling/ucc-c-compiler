// RUN: %ocheck 0 %s

typedef struct A T;
T *ext;

main()
{
	printf("%p\n", ext);
}
