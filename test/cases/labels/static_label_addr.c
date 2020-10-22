// RUN: %ocheck 5 %s

/* static void *x = &&label;
 * unary-&& had never been used in static context,
 * and const_fold doesn't know what the basic-block
 * will be labelled.
 *
 * Thus we force an extra label emission for blocks who
 * are unary-&&'d.
 *
 * Tested twice to ensure unique label asm names
 */

f()
{
	static void *p = &&L;

	goto *p;
L:
	return 5;
}

main()
{
	static void *p = &&L;

	goto *p;

L: return f();
}
