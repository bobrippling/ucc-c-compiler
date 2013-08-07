// RUN: %caret_check %s

f(int *a, int *b, int c)
{
	return a && b || c;
// CARETS:
//             ^ warning: && has higher precedence than ||
}
