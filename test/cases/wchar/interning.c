// RUN: %layout_check %s

typedef int wchar_t;

wchar_t *a = L"hello";
char *b = "hello";
wchar_t *c = L"hello";
char *d = "hello";
