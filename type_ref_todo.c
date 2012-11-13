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
_Noreturn void f() // CHECK: /function "f" marked no-return implicitly returns/
{
}

extern int __attribute__((warn_unused)) use_proper();

extern int use() __attribute__((warn_unused));
/*
 * __attribute__ on the end is a special case for the parser
 *
 * since it could be:
 *
 * int use()
 *   __attribute__((a)) int i;
 *   int j;
 * {
 * }
 *
 * ... etc
 */

doesnt_use()
{
	use_proper(); // CHECK: /unused expression/

	use(); // CHECK: /unused expression/

	__builtin_trap();
}

#endif
