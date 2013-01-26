// RUN: %ucc %s

_Noreturn struct A *p()
{
	for(;;);
}
