// RUN: %ucc %s 2>&1 | head -4 | %stdoutcheck %s

# line 5 "yo.h"

# 2 "inc.c"

# 8 "yo2.h"

compile error;
# 3 "inc.c"

main()
{
}

// STDOUT: /inc.c:2: included from here/
// STDOUT-NEXT: yo.h:5: included from here
// STDOUT-NEXT: inc.c:2: included from here
// STDOUT-NEXT: yo2.h:9:1: error: unknown type name 'compile'
