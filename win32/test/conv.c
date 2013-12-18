#define _cdecl    __attribute((cdecl))
#define _stdcall  __attribute((stdcall))
#define _fastcall __attribute((fastcall))

#define F(a, f)              \
int a f(int x, int y, int z) \
{                            \
	return x + y + z;          \
}

F(_cdecl,    cdecl)
F(_stdcall,  stdcall)
F(_fastcall, fastcall)

int normal(x, y, z)
{
	return x + y + z;
}

main()
{
	cdecl(1, 2, 3);
	stdcall(1, 2, 3);
	fastcall(1, 2, 3);
	normal(1, 2, 3);
}
