// RUN: %caret_check %s -Wassign-in-test

main()
{
int a = 0, *b = (void *)0, c = 0;

a = b;
// CARETS:
//^ warning: mismatching types

if(a = c);
// CARETS:
//   ^ assignment in if
while(a = c);
// CARETS:
//      ^ assignment in while
do ; while(a = c);
// CARETS:
//           ^ assignment in do
for(;a = c;);
// CARETS:
//     ^ assignment in for-test
}
