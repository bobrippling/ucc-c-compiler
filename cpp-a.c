// XXX: the filename of this file must be 7 chars, otherwise the bug doesn't appear
// e.g. scope.c
#include <assert.h>

int
f1 (int i)
{
    struct foo { int m; };
    if (i > sizeof(struct foo { int m; }))
        return sizeof(struct foo { int m; });
    else
        return sizeof(struct foo);
}

int
f2 (void)
{
    struct foo { int m; };
    for (struct foo { int m; } x;;)
        return sizeof(struct foo { int m; });
}

int
f3 (void)
{
    struct foo { int a; };
    while (sizeof(struct foo { int a; }))
        return sizeof(struct foo { int m; });
}

int
f4 (int i)
{
    for (struct foo *x = (struct foo { int m; }*)&i;;)
        return x->m + sizeof(struct foo { int m; });
}

int
main (void)
{
    assert(f1(1) == sizeof(int));
    assert(f2() == sizeof(int));
    assert(f3() == sizeof(int));
    assert(f4(9) == 9 + sizeof(int));
}

/*
=================================================================
==20144==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x60c000007205 at pc 0x55ebc11d4d0d bp 0
x7ffc6e5239e0 sp 0x7ffc6e5239d8
READ of size 1 at 0x60c000007205 thread T0
    #0 0x55ebc11d4d0c in eval_expand_macros /home/rob/code/c/Programs/ucc/src/cpp2/eval.c:466
    #1 0x55ebc11c857d in filter_macros /home/rob/code/c/Programs/ucc/src/cpp2/preproc.c:407
    #2 0x55ebc11c8729 in preprocess /home/rob/code/c/Programs/ucc/src/cpp2/preproc.c:430
    #3 0x55ebc11cb47a in main /home/rob/code/c/Programs/ucc/src/cpp2/main.c:664
    #4 0x7f65f59afd09 in __libc_start_main ../csu/libc-start.c:308
    #5 0x55ebc11c6519 in _start (/home/rob/code/c/Programs/ucc/src/cpp2/cpp+0xe519)

0x60c000007205 is located 5 bytes to the right of 128-byte region [0x60c000007180,0x60c000007200)
allocated by thread T0 here:
    #0 0x7f65f5c55a2e in __interceptor_realloc (/usr/lib/x86_64-linux-gnu/libasan.so.5+0x107a2e)
    #1 0x55ebc11d9ee3 in urealloc /home/rob/code/c/Programs/ucc/src/util/alloc.c:28
    #2 0x55ebc11da1ac in ustrvprintf /home/rob/code/c/Programs/ucc/src/util/alloc.c:84
    #3 0x55ebc11da39a in ustrprintf /home/rob/code/c/Programs/ucc/src/util/alloc.c:101
    #4 0x55ebc11cd30d in str_replace /home/rob/code/c/Programs/ucc/src/cpp2/str.c:130
    #5 0x55ebc11cd361 in word_replace /home/rob/code/c/Programs/ucc/src/cpp2/str.c:139
    #6 0x55ebc11d4414 in eval_macro_r /home/rob/code/c/Programs/ucc/src/cpp2/eval.c:354
    #7 0x55ebc11d4886 in eval_macro_double_eval /home/rob/code/c/Programs/ucc/src/cpp2/eval.c:407
    #8 0x55ebc11d4a64 in eval_macro /home/rob/code/c/Programs/ucc/src/cpp2/eval.c:435
    #9 0x55ebc11d4cd0 in eval_expand_macros /home/rob/code/c/Programs/ucc/src/cpp2/eval.c:464
    #10 0x55ebc11d48de in eval_macro_double_eval /home/rob/code/c/Programs/ucc/src/cpp2/eval.c:415
    #11 0x55ebc11d4a64 in eval_macro /home/rob/code/c/Programs/ucc/src/cpp2/eval.c:435
    #12 0x55ebc11d4cd0 in eval_expand_macros /home/rob/code/c/Programs/ucc/src/cpp2/eval.c:464
    #13 0x55ebc11d48de in eval_macro_double_eval /home/rob/code/c/Programs/ucc/src/cpp2/eval.c:415
    #14 0x55ebc11d4a64 in eval_macro /home/rob/code/c/Programs/ucc/src/cpp2/eval.c:435
    #15 0x55ebc11d4cd0 in eval_expand_macros /home/rob/code/c/Programs/ucc/src/cpp2/eval.c:464
    #16 0x55ebc11c857d in filter_macros /home/rob/code/c/Programs/ucc/src/cpp2/preproc.c:407
    #17 0x55ebc11c8729 in preprocess /home/rob/code/c/Programs/ucc/src/cpp2/preproc.c:430
    #18 0x55ebc11cb47a in main /home/rob/code/c/Programs/ucc/src/cpp2/main.c:664
    #19 0x7f65f59afd09 in __libc_start_main ../csu/libc-start.c:308

SUMMARY: AddressSanitizer: heap-buffer-overflow /home/rob/code/c/Programs/ucc/src/cpp2/eval.c:466 in eval_
expand_macros
Shadow bytes around the buggy address:
  0x0c187fff8df0: fd fd fd fd fd fd fd fd fa fa fa fa fa fa fa fa
  0x0c187fff8e00: fd fd fd fd fd fd fd fd fd fd fd fd fd fd fd fd
  0x0c187fff8e10: fa fa fa fa fa fa fa fa fd fd fd fd fd fd fd fd
  0x0c187fff8e20: fd fd fd fd fd fd fd fd fa fa fa fa fa fa fa fa
  0x0c187fff8e30: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
=>0x0c187fff8e40:[fa]fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa
  0x0c187fff8e50: fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa
  0x0c187fff8e60: fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa
  0x0c187fff8e70: fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa
  0x0c187fff8e80: fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa
  0x0c187fff8e90: fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa
Shadow byte legend (one shadow byte represents 8 application bytes):
  Addressable:           00
  Partially addressable: 01 02 03 04 05 06 07
  Heap left redzone:       fa
  Freed heap region:       fd
  Stack left redzone:      f1
  Stack mid redzone:       f2
  Stack right redzone:     f3
  Stack after return:      f5
  Stack use after scope:   f8
  Global redzone:          f9
  Global init order:       f6
  Poisoned by user:        f7
  Container overflow:      fc
  Array cookie:            ac
  Intra object redzone:    bb
  ASan internal:           fe
  Left alloca redzone:     ca
  Right alloca redzone:    cb
  Shadow gap:              cc
==20144==ABORTING
*/
