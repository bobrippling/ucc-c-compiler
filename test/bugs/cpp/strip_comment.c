#define typename(x) _Generic((x),                                                 \
        _Bool: "_Bool",                  unsigned char: "unsigned char",          \
         char: "char",                     signed char: "signed char",            \
       void *: "pointer to void",                int *: "pointer to int",         \
      default: "other")
