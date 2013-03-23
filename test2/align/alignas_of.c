main()
{
#define PALIGN(ty) \
	printf("%d %d\n", _Alignof(ty), \
			__alignof__(ty))

	PALIGN(int);
	PALIGN(short);

	_Alignas(__alignof__(int)) short x = 2;
	_Alignas(4) char c;
}
