/* Except when it is the operand of the sizeof operator, the unary & operator,
 * the ++ operator, the -- operator, or the left operand of the . operator or
 * an assignment operator, an lvalue that does not have array type is converted
 * to the value stored in the designated object (and is no longer an lvalue).
 * If the lvalue has qualified type, the value has the unqualified version of
 * the type of the lvalue; otherwise, the value has the type of the lvalue. If
 * the lvalue has an incomplete type and does not have array type, the behavior
 * is undefined. If the lvalue designates an object of automatic storage
 * duration that could have been declared with the register storage class
 * (never had its address taken), and that object is uninitialized (not
 * declared with an initializer and no assignment to it has been performed
 * prior to use), the behavior is undefined.
 */

main()
{
	int i;
	struct
	{
		int x, y;
	} a ;//, g();

	i = 3;

	f(&i);
	f(i++);
	f(++i);
	f(i--);
	f(--i);
	f(a.x);

	const int ki = 3;
	_Static_assert(
			__builtin_types_compatible_p(
				typeof(ki),
				int),
			"decay?");
}
