// RUN: %ucc -c -fcommon -o %t.1.o %s -DMAIN
// RUN: %ucc -c -fcommon -o %t.2.o %s
// RUN: %ucc -o %t %t.1.o %t.2.o

int i;

#ifdef MAIN
main()
{
	return i;
}
#endif
