// RUN: %check -e %s

char b[] = "\xq"; // CHECK: error: empty escape sequence
int c3 = L'\x123456789'; // CHECK: error: escape character out of range
int s3[] = L"\x123456789"; // CHECK: error: escape sequence out of range
char of[] = "\x100"; // CHECK: error: escape sequence out of range
