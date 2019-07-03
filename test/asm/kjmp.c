// RUN: %ucc -S -o %t %s
// RUN: grep -v test %t

main()
{
	if(0)
		f();
}
