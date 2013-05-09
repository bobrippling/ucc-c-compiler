// RUN: %ucc -c %s
// RUN: %check %s

_Noreturn void f() // CHECK: /function "f" marked no-return implicitly returns/
{
}

extern int __attribute__((warn_unused)) use_proper();

extern int use() __attribute__((warn_unused));

doesnt_use()
{
	use_proper(); // CHECK: /unused expression/
	use(); // CHECK: /unused expression/
	__builtin_trap();
}
