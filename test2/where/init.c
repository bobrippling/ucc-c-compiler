// RUN: %caret_check %s

int i = (void *)0;
// CARETS:
//    ^ note:

// not a top-level init:
int x[] = { (void *)0, 2 };
// CARETS:
//          ^ note:
