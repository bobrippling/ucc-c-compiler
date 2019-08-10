// RUN: %layout_check %s
char x[] = "hello";
char *p = "yo";

__asm("_" " there");
__asm("" "yo");

const char *const joined = "yo" "second";
const char joined_a[] = "hi" "two";

//__asm(L"_" " there");
//__asm("_" L" there");
//__asm(L"_" L" there");

const int *const pWIDE1 = L"yo" "second";
const int *const pWIDE2 = L"yo" L"second";
const int *const pWIDE3 = "yo" L"second";
const int aWIDE1[] = L"hi" "two";
const int aWIDE2[] = L"hi" L"two";
const int aWIDE3[] = "hi" L"two";
