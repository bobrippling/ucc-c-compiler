// RUN: %caret_check %s

typedef unsigned long size_t;

void *func(void *const restrict arg1, const void *restrict const arg2,
// CARETS:
//    ^ control reaches end of non
//                              ^ "arg1" never read
//                                                               ^ "arg2" never read

		size_t arg3)
// CARETS:
//     ^ "arg3" never read
// note: this goes off the "size_t arg3" string, i.e. ltrim()'d
{
}
