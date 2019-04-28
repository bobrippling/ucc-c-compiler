// TEST: target darwin
// RUN: %ucc -x cpp-output -fsyntax-only %s

# 1 "variadic/darwin_workaround.c" 1

# 1 "types.h" 1 3
typedef void *voidp;
typedef voidp va_list;

typedef void *va_list;

# 2 "variadic/darwin_workaround.c" 2

int main()
{
	va_list l;
	l->reg_save_area;
}
