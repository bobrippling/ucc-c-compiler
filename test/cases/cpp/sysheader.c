#include <sys.h> // tagged as a sys  header since it's in -isystem
#include <user.h> // tagged as a user header since it's in -I

// RUN: %ucc -E -nostdinc -isystem cases/cpp/sysinc -I cases/cpp/userinc %s -o %t
// RUN: grep '# 1 ".*/sysheader.c" 1'    %t
// RUN: grep '# 1 ".*/sysinc/sys.h" 1 3' %t
// RUN: grep '# 2 ".*/sysheader.c" 2'    %t
// RUN: grep '# 1 ".*/userinc/user.h" 1' %t
// RUN: grep '# 3 ".*/sysheader.c" 2'    %t
