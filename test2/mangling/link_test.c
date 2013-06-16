// RUN: %ucc -o %t %s
// RUN: L_U=`uname -s|grep >/dev/null -iE '(cygwin|darwin)' && echo 1 || echo 0`; %t; [ $? -eq $L_U ]

main()
{
	printf("hello there %d %s 0x%x\n",
			23, "yo yo", 0x32);

	return __LEADING_UNDERSCORE;
}
