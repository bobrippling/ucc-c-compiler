// RUN: %caret_check %s

main()
{
	int i;

i = 3; _Alignof(int);
// CARETS:
//     ^ unused expression
}
