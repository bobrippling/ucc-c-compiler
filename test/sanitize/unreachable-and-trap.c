// RUN:   %ucc -S -o- %s -fsanitize=unreachable -DF=__builtin_trap        | grep ud2 >/dev/null
// RUN:   %ucc -S -o- %s                        -DF=__builtin_trap        | grep ud2 >/dev/null
// RUN:   %ucc -S -o- %s -fsanitize=unreachable -DF=__builtin_unreachable | grep ud2 >/dev/null
// RUN: ! %ucc -S -o- %s                        -DF=__builtin_unreachable | grep ud2 >/dev/null

void f()
{
	F();
}
