// RUN: %ucc %s
// just need to ensure it links - single string symbol

f(){}

main()
{
	printf("yo\n");
	if(f())
		;
}
