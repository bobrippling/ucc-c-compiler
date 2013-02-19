#include <wchar.h>

_main()
{
	const wchar_t *s = L"ABCDEF";

	_wprintf(L"Yo\n");

	// TODO: return L'a';
	return s[1]; // 66
}
