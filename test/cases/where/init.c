// RUN: %caret_check %s -pedantic

int i = (void *)0;
// CARETS:
//      ^ warning: mismatching types, initialisation

// not a top-level init:
int x[] = { (void *)0, 2 };
// CARETS:
//          ^ warning: mismatching types, initialisation
