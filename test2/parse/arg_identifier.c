// gcc treats this as an error, as does tcc. clang accepts
// RUN: %check -e %s

f(int i, int j, va_list); // CHECK: /error: type expected/

main()
{
	f(1, 2, 3);
}
