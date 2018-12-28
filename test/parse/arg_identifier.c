// gcc treats this as an error, as does tcc. clang accepts
// RUN: %check %s

f(int i, int j, va_list); // CHECK: !/warn/

main()
{
	f(1, 2, 3);
}
