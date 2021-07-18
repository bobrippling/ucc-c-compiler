#define DOG(x) /* yo */ x
const char s[] = "hello" /* yo */ "there";

DOG(3)

// $ cc -E -C %
// 1) will not properly protect the comment in the macro
// 2) cc1 can't join strings separated by a comment
