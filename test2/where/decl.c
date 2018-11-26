// RUN: %caret_check %s -Wsym-never-read

typedef unsigned long size_t;

// beware the trimming logic
void *func(void *const restrict arg1, void *arg2,
// CARETS:
//    ^ control reaches end of non
//                              ^ "arg1" never read
//                                          ^ "arg2" never read

		size_t arg3)
// CARETS:
//     ^ "arg3" never read
// note: this goes off the "size_t arg3" string, i.e. ltrim()'d
{
}
