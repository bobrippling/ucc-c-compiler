// RUN: %ocheck 4 %s

main()
{
	typedef int int_t;
	goto int_t;

int_t
	// hello
	/* making it
#difficult
*/
#line 50
	:
	return 4;
}
