// RUN: %caret_check %s

void *const y(int);
// CARETS:
//    ^ const qualification on return type has no effect
