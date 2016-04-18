// RUN: %check %s

__attribute((noreturn)) void abort(void);

_Noreturn auto f() -> char * // CHECK: warning: function "f" marked no-return has a non-void return value
{
	abort();
}
