// RUN: %layout_check %s
typedef unsigned long ul;
int ent1;
ul ent2 = (ul)&ent1;
