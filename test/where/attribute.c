// RUN: %caret_check %s


int x __attribute((abc));
// CARETS:
//                 ^ ignoring unrec
