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

//char32_t incompat1[] = L"hi";

int main()
{
    char s1[] = "a猫🍌"; // aka "a\u732B\U0001F34C"
    char s2[] = u8"a猫🍌";
    char16_t s3[] = u"a猫🍌";
    char32_t s4[] = U"a猫🍌";
    wchar_t s5[] = L"a猫🍌";
}
