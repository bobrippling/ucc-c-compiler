// RUN: %caret_check %s


void *f_name(void *p_name, int a_name)
{
// operator
p_name++    ;
// CARETS:
//    ^ arithmetic on void pointer

0,  ++p_name    ;
// CARETS:
//  ^ arithmetic on void pointer

return p_name += a_name;
// CARETS:
//            ^ arithmetic on void pointer
}

// initialisation
int *p_name = (char *)0;
// CARETS:
//            ^ warning

// init 2
int func()
// CARETS:
//  ^ control reaches
{
void q();
(void)(1 ? f_name : q);
// CARETS:
//       ^ conditional type mismatch
}

main()
{
// argument
f_name((void *)0, (int *)5);
// CARETS:
//                ^ warning

int integer = 2;
f_name(integer, 3);
// CARETS:
//     ^ warning
}

f(int *);

func2()
{
int abcdef = 0;
f(1234);
// CARETS:
//^ warning
f(abcdef);

char c;
char *p = 0;
f(c);
// CARETS:
//^ warning
f(&c);
// CARETS:
//^ warning
f(  *p);
// CARETS:
//  ^ warning
f(5 + 2);
// CARETS:
//  ^ warning

f(    sizeof(__typeof(int)));
// CARETS:
//    ^ warning
}
