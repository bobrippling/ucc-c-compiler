// RUN: ! %ucc %s
main()
{
	q();
	^
		a = (short *)5;
}
