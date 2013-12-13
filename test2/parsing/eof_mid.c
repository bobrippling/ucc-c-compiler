// RUN: %ucc %s; [ $? -ne 0 ]
main()
{
	q();
	^
		a = (short *)5;
}
