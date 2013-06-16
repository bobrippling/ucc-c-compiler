// RUN: %ucc %s 2>&1 | head -2 | %output_check 'inc.c:2: included from here' "yo2.h:9:13: error: unknown type name 'compile'"

# line 5 "yo.h"

# 2 "inc.c"

# 8 "yo2.h"

compile error;
# 3 "inc.c"

main()
{
}
