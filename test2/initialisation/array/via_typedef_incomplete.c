// RUN: %layout_check %s

typedef int A[];
A ent1 = { 1, 2 },
  ent2 = { 3, 4, 5 };
