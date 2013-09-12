// RUN: %caret_check %s

main()
{
	while(){}
// CARETS:
//    ^ error: expression expected
}
