from_stack()
{
	/* we save a pointer to the stack
	 * this should be stored as type `int'
	 * i.e. no multiplication before adding to f()
	 */
	return *(int *)0 + f();
}
