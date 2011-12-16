int zero_noinit;        // .bss
int zero = 0;           // .bss
int zero_cast = (int)0; // .bss   FIXME
int non_zero = 1;       // .data
int *p = &non_zero;     // .data

main(){}
