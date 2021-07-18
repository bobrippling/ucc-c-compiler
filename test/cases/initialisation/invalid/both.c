// RUN: %ucc -c -DAR %s; [ $? -ne 0 ]
// RUN: %ucc -c      %s; [ $? -ne 0 ]

#ifdef AR
int x[] = 1;
#else
union { int i; } x = 5;
#endif
