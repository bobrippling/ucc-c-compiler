// RUN: %check %s
// RUN: %check --prefix=err -e %s -DERR

#ifdef ERR
extern char x[32]; // CHECK-err: !/error/
char x[32]; // CHECK-err: !/error/

extern char y[32]; // CHECK-err: /note: previous definition/
char y[16]; // CHECK-err: /error: mismatching definitions of "y"/
#endif


extern int ptrs[4];
int ptrs[] = {}; //- valid (ignoring GNU init)
_Static_assert(sizeof(ptrs) == sizeof(int) * 4, "");

extern int ptrs2[4];
int ptrs2[] = { 0 }; //- valid
_Static_assert(sizeof(ptrs2) == sizeof(int) * 4, "");

extern int ptrs3[4];
int ptrs3[] = {1,2,3,4,5}; // CHECK: warning: excess initialiser for 'int[4]'
_Static_assert(sizeof(ptrs3) == sizeof(int) * 4, "");

extern int ptrs4[];
int ptrs4[] = {1,2,3,4,5};
_Static_assert(sizeof(ptrs4) == sizeof(int) * 5, "");

extern int ptrs5[];
int ptrs5[]; // CHECK: warning: tenative array definition assumed to have one element
#ifdef ERR
_Static_assert(sizeof(ptrs5) == sizeof(int), ""); // CHECK-err: error: sizeof incomplete type int[]
#endif
