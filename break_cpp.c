#define typename(x) _Generic((x),                                                 \
        _Bool: "_BoolÓ,                  unsigned char: "unsigned charÓ,          \
         char: "char",                     signed char: "signed char",            \
       void *: "pointer to voidÓ,                int *: "pointer to int",         \
      default: "other")
