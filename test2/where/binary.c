// RUN: %caret_check %s -pedantic

main()
{
printf("%d\n", 0b110);
// CARETS:
//             ^ warning: binary literals are an extension
}
