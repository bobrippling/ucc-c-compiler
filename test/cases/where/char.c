// RUN: %caret_check %s -std=c89

main()
{
'abc'; f(); "";
// CARETS:
//          ^ warning: unused expression \(string\)
//     ^ warning: implicit declaration of function "f"
}
