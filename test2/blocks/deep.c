// RUN: %ucc -c %s
main()
{
	typedef int (*(*(*(*fp)())())())();
	({fp this= ^{return 0;}; this;})()()()();
}
