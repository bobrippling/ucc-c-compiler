// RUN: %check -e %s

typedef int wchar_t;

wchar_t *pw = "hello"; // CHECK: /warning: mismatching types/
char *pc = L"hello"; // CHECK: /warning: mismatching types/

wchar_t aw[] = "hello"; // CHECK: /error: incorrect string literal/
char ac[] = L"hello"; // CHECK: /error: incorrect string literal/
