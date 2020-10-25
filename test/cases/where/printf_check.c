// RUN: %caret_check %s

main()
{
int printf(char *, ...) __attribute((format(printf,1,2)));

printf(   "%Q\n");
// CARETS:
//          ^ warning: invalid conversion character
}
