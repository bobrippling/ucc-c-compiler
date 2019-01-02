// RUN: %caret_check %s

main()
{
  int defined();

defined(implicit());
// CARETS:
//      ^ warning: implicit
}
