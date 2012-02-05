main()
{
//#define uint unsigned
	typedef unsigned uint;
	uint i;

	i > 5; // should emit "seta", not "setg"
}
