// RUN: %caret_check %s

char x[] = "hello \. yo";
// CARETS:
//                ^ invalid escape character
