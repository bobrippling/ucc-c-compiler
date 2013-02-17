#include <wchar.h>

main()
{
	const wchar_t *s = L"ABCDEF";

	wprintf(L"Yo\n");

	// TODO: return L'a';
	return s[1]; // 66
}
