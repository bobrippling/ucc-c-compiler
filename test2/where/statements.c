// RUN: %caret_check %s

main()
{
int a = 0, *b = (void *)0, c = 0;

a = b;
// CARETS:
//^ note:

if(a = c);
// CARETS:
//   ^ assignment in
while(a = c);
// CARETS:
//      ^ assignment in
do ; while(a = c);
// CARETS:
//           ^ assignment in
for(;a = c;);
// CARETS:
//     ^ assignment in
}
