// RUN: %ucc -'###' -D x=y -U x -I path1 -L path2 -l lib a.c >%t 2>&1
// RUN: grep 'cpp.* -Dx=y' %t
// RUN: grep 'cpp.* -Ux' %t
// RUN: grep 'cpp.* -Ipath1' %t
// RUN: grep 'ld.* -Lpath2' %t
// RUN: grep 'ld.* -llib' %t
