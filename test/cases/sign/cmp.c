// RUN: %check %s

main()
{
	unsigned i = 0;
	if(i >= 0); // CHECK: /comparison of unsigned expression >= 0 is always true/
	if(i < 0);  // CHECK: /comparison of unsigned expression < 0 is always false/
}
