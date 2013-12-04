// RUN: %check %s
// RUN: %ucc -c %s -o %t

enum
{
	A = 'abcde' // CHECK: /warning: multi-char constant too large/
};

main()
{
	printf("%d\n", A);
}
