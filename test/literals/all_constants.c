// RUN: %ucc -fsyntax-only %s

main()
{
	1;
	1U;
	1L;
	1LL;

	1UL;
	1ULL;

	1LL;
	1LLU;

	//

	2.3e-5;
	-5.2f;
	1.L;
	2.F;

	0x52.1p-3f;
}
