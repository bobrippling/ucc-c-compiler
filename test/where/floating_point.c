// RUN: %caret_check %s -std=c89

int i=_Generic(2., __typeof(f(), 1.2l):(char)1+g());
// CARETS:
//                          ^ warning: .*implicit.*function "f"
//                                             ^ warning: .*implicit.*function "g"
