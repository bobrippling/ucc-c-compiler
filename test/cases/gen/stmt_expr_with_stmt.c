// RUN: %ucc -fsyntax-only %s

main()
{
	({
	 (void)0;
	 });
	({
	 return 5;
	 });
}
