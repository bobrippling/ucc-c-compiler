// RUN: %ocheck 0 %s
// RUN: %ucc -o %t %s
// RUN: %t | %output_check 'Yo'
void abort(void) __attribute__((noreturn));

//#include <wchar.h>
typedef __WCHAR_TYPE__ wchar_t;

main()
{
	const wchar_t *s = L"ABCDEF";

	wprintf(L"Yo\n");

	if(L'A' + s[1] != 131) // 65 + 66
		abort();
	return 0;
}
