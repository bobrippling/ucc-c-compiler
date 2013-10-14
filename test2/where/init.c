// RUN: %caret_check %s

int i = (void *)0;
// CARETS:
//    ^ mismatching

// not a top-level init:
int x[] = { (void *)0, 2 };
// CARETS:
//          ^ mismatching
