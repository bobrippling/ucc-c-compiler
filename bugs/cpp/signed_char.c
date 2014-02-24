#if '\0' - 1 > 0
char signed
#else
char unsigned
#endif

#if L'\0' - 1 > 0
wchar signed
#else
wchar unsigned
#endif
