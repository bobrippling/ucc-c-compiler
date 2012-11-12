#ifdef ONE
enum
{
	A, B
};
enum reg
{
	RAX,
	RBX,
	RCX,
	RDX,

	RDI,
	RSI,

	R8,
	R9,
	R10,
	R11,
}; // complains no-declaring decl
#endif

#ifdef TWO
_Noreturn f()
{
	__attribute((noreturn)) q();
	q(); // noreturn attribute isn't picked up (attributes in general)
}
#endif
