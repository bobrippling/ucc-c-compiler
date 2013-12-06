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
int *p_name = 0;
// CARETS:
//          ^ note:

// init 2
int func()
// CARETS:
//  ^ control reaches
{
void q();
(void)(1 ? f_name : q);
// CARETS:
//       ^ pointer type mismatch
}

main()
{
// argument
f_name((void *)0, (int *)5);
// CARETS:
//                ^ note:

int integer = 2;
f_name(integer, 3);
// CARETS:
//     ^ note:
}

f(int *);

func2()
{
int abcdef = 0;
f(1234);
// CARETS:
//^ note:
f(abcdef);

char c;
char *p = 0;
// CARETS:
//      ^ note:
f(c);
// CARETS:
//^ note:
f(&c);
// CARETS:
//^ note:
f(  *p);
// CARETS:
//  ^ note:
f(5 + 2);
// CARETS:
//  ^ note:

f(    sizeof(typeof(int)));
// CARETS:
//    ^ note:
}
