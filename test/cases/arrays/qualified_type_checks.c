// RUN: %ucc -fsyntax-only %s

typedef int array[3];

const array yo; // int const yo[3];
//                             ^~ no const here

h(array); // int h(int [3]);

i(const array); // int i(int const [3]);
//                                 ^~ no const here

j(int[const]); // int j(int *const);


_Static_assert(_Generic(yo, const int[3]: 1) == 1, "");

_Static_assert(_Generic(&h, int (*)(int[3]): 1) == 1, "");
_Static_assert(_Generic(&h, int (*)(int const[3]): 1, default: 2) == 2, "");
_Static_assert(_Generic(&h, int (*)(int *const): 1) == 1, "");
_Static_assert(_Generic(&h, int (*)(int *): 1) == 1, "");

_Static_assert(_Generic(&i, int (*)(int const[3]): 1) == 1, "");
_Static_assert(_Generic(&i, int (*)(int      [3]): 1, default: 2) == 2, "");
_Static_assert(_Generic(&i, int (*)(int const *): 1) == 1, "");
_Static_assert(_Generic(&i, int (*)(int const *const): 1) == 1, "");

_Static_assert(_Generic(&j, int (*)(int *const): 1) == 1, "");
_Static_assert(_Generic(&j, int (*)(int [const]): 1) == 1, "");
_Static_assert(_Generic(&j, int (*)(int const *const): 1, default: 2) == 2, "");
_Static_assert(_Generic(&j, int (*)(int const [const]): 1, default: 2) == 2, "");
