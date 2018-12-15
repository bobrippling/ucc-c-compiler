// RUN: %caret_check %s

f(int);

main()
{
(void)12345, f((void *)0);
// CARETS:
//             ^ warning
(void)0x12345, f((void *)0);
// CARETS:
//               ^ warning
(void)0b1010, f((void *)0);
// CARETS:
//              ^ warning
(void)017164113, f((void *)0);
// CARETS:
//                 ^ warning
(void)0, f((void *)0);
// CARETS:
//         ^ warning
}
