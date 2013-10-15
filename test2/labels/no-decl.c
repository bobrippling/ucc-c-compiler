// RUN: %check -e %s

main()
{
	goto a; // CHECK: /error: label 'a' undefined/
}
