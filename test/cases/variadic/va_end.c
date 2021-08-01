// RUN: %ucc -fsyntax-only %s
int f(__builtin_va_list l)
{
	__builtin_va_end(l);
}
