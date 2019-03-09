// RUN: %caret_check %s

__auto_type x = ^{ printf("hi\n"}; };
// CARETS:
//                              ^ error: expecting token '\)', got '}'
