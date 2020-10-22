// RUN: %layout_check %s
typedef int tdef_int;

typedef tdef_int tdef_int2;

tdef_int2 i = 3;


typedef int *intptr;
intptr p = (void *)0;

__attribute((used))
static intptr x;
