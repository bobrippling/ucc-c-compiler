int main(void)
{
	int foo = 10, bar = 15;

	/* --setup--
	 * foo=10
	 * bar=15
	 * load foo/10 -> eax
	 * load bar/15 -> ebx
	 */

	__asm__("addl %%ebx, %%eax"
			: "=a"(foo)
			: "a"(foo), "b"(bar));

	/* --tidy--
	 * store eax -> foo
	 */

	return foo;
}
