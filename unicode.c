#include <stddef.h> /* wchar_t */
#include <uchar.h>

wchar_t  hiw[] = L"hi";
char16_t hi16[] = u"hi";
char32_t hi32[] = U"hi";
char     hi8[] = u8"hi";

wchar_t  hw = L'h';
char16_t h16 = u'h';
char32_t h32 = U'h';
//char     h8 = u8'h';


char32_t incompat1[] = L"hi";
