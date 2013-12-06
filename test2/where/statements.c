// RUN: %caret_check %s

main()
{
int a = 0, *b = (void *)0, c = 0;

a = b;
// CARETS:
//^ note:

if(a = c);
// CARETS:
//   ^ testing an assignment
while(a = c);
// CARETS:
//      ^ testing an assignment
do ; while(a = c);
// CARETS:
//           ^ testing an assignment
for(;a = c;);
// CARETS:
//     ^ testing an assignment

return _Generic(0, char: 5);
// CARETS:
//     ^ no type satisfying
}
