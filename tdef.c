enum
{
	THAT_WHICH_IS_THREE = 2
};

main()
{
	typedef int tdef_int;
	typedef tdef_int tdef_int2;

	tdef_int2 x;
	x = THAT_WHICH_IS_THREE;
	return x;
}
