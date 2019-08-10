// RUN: %ucc -o %t %s
// must ensure we link successfully

main()
{
	static int i = 3;

	const char *s;
	s = "AAA";
	if(!s)
		s = "BBB";

	if((struct A { int i, j; } *)0, s = "CCC")
		;

	if(char *p = "DDD") // if-stmt has a new symbol table
		;

	if(char *p = "EEE", p[1]) // also new symtable here
		;
}
