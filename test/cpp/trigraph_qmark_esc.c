// RUN: %ucc -trigraphs %s -o %t
// RUN %t | grep -F 'what??!'

main()
{
	printf("what?\?!\n");
}
