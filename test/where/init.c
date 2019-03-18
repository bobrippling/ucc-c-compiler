// RUN: %caret_check %s -pedantic

int i = (void *)0;
// CARETS:
//    ^ warning: global scalar initialiser contains non-standard constant expression
//      ^ note: cast expression here

// not a top-level init:
int x[] = { (void *)0, 2 };
// CARETS:
//      ^ warning: global brace initialiser contains non-standard constant expression
//          ^ note: cast expression here
