// RUN: %ocheck 0 %s

typedef int tdef;

main()
{
	// tdef: looks like a label, but we want to interpret it as a type
	int i = _Generic(5, tdef: 0);

	return i;
}
