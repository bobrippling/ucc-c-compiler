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
//            ^ mismatching

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
//                ^ mismatching arg

int integer = 2;
f_name(integer, 3);
// CARETS:
//     ^ mismatching arg
}

f(int *);

func2()
{
int abcdef = 0;
f(1234);
// CARETS:
//^ mismatching arg
f(abcdef);

char c;
char *p = 0;
// CARETS:
//        ^ mismatching types
f(c);
// CARETS:
//^ mismatching arg
f(&c);
// CARETS:
//^ mismatching arg
f(  *p);
// CARETS:
//  ^ mismatching arg
f(5 + 2);
// CARETS:
//  ^ mismatching arg

f(    sizeof(typeof(int)));
// CARETS:
//    ^ mismatching arg
}
