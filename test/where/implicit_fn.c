// RUN: %caret_check %s -std=c89

main()
{
  int defined();

defined(implicit());
// CARETS:
//      ^ warning: implicit
}
