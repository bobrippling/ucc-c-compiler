#define JOIN_(a, b) a ## b
#define JOIN(a, b) JOIN_(a, b)

#define SYMBL(x) JOIN(__USER_LABEL_PREFIX__, x)
