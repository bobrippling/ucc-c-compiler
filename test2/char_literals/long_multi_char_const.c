// RUN: %check %s
// RUN: %ucc -c %s -o %t

main()
{
	int a = 'abcde'; // CHECK: /warning: multi-char constant too large/
	printf("%d\n", a);
}
