// RUN: %check %s

enum
{
	A = 'abcde' // CHECK: /warning: multi-char constant too large/
};

main()
{
	printf("%d\n", A);
}
