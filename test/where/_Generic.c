// RUN: %caret_check %s

main()
{
return _Generic(0, char: 5);
// CARETS:
//     ^ no type satisfying
}
