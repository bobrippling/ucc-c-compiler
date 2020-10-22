// RUN: %ucc -fsyntax-only %s

unsigned long strlen(const char *);
char x[strlen("yo") - 1];
_Static_assert(sizeof(x) == 1, "bad strlen-k");
