// RUN: %caret_check %s

char x[] = "hello \. yo";
// CARETS:
//                ^ unrecognised escape character '.'
