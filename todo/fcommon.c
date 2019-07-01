//__attribute((visibility("hidden")))
int a, b;
int f(void) { return a + b; }

/*
https://gcc.gnu.org/pipermail/gcc-help/2019-May/135621.html
(partly related) https://gcc.gnu.org/bugzilla/show_bug.cgi?id=70146

-m64
-fcommon and -fno-common both generate the same loads

-m32 -fno-common
  call    __x86.get_pc_thunk.dx
  addl    $_GLOBAL_OFFSET_TABLE_, %edx
  movl    b@GOTOFF(%edx), %eax
  addl    a@GOTOFF(%edx), %eax


-m32 -fcommon
  call    __x86.get_pc_thunk.ax
  addl    $_GLOBAL_OFFSET_TABLE_, %eax
  movl    a@GOT(%eax), %edx
  movl    b@GOT(%eax), %eax
  movl    (%eax), %eax
  addl    (%edx), %eax


tl;dr: on 32-bit we may have to treat common symbols as extern
*/
