// RUN: %ucc -o %t %s

f(){}

// ensure it links
main()
{
	f("hex dec\n");
	for(int i;;);
}
