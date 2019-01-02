// RUN: %caret_check %s

f(int *a, int *b, int c)
{
 return a && b || c; // this is ltrim'd
// CARETS:
//            ^ warning: && has higher precedence than ||
}
