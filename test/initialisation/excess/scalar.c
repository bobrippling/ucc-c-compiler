// RUN: %check %s
// RUN: %caret_check %s

int x = { 1, 2 }; // CHECK: /warning: excess initialiser for 'int'/
// CARETS:
//           ^ warning: excess

char b = { 1, 2, 3 }; // CHECK: /warning: excess initialiser for 'char'/
// CARETS:
//            ^ warning: excess

main()
{
return (int){ 5, 6 }; // CHECK: /warning: excess initialiser for 'int'/
// CARETS:
//               ^ warning: excess
}
