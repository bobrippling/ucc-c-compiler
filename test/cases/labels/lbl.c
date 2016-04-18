// RUN: %ocheck 0 %s

main()
{
	goto a; // forward decl in nested scope

	{ b: goto c; } // forward decl

d: return 0;

c: goto d; // backwards / pre-existing decl

	// backwards into nested scope
	{ a: goto b; }
}
