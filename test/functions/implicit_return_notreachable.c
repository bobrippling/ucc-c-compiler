// RUN: %check %s

// should figure out that we don't pass the "while" statement / fall of the end of the function

ret() // CHECK: !/warn/
{
	while(1)
		return 2;
}
