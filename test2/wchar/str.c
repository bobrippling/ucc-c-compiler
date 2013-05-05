// RUN: %ocheck 66 %s
// RUN: %ucc -o %t %s
// RUN: %t | %output_check 'Yo'
#include <wchar.h>

main()
{
	const wchar_t *s = L"ABCDEF";

	wprintf(L"Yo\n");

	// TODO: return L'a';
	return s[1]; // 66
}
