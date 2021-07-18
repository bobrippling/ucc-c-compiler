// RUN: LIBRARY_PATH=:::hello:::there:: %ucc -### a.c >%t 2>&1
// RUN:   grep '^ld.* -Lhello' %t >/dev/null
// RUN:   grep '^ld.* -Lthere' %t >/dev/null
// RUN: ! grep '^ld.* -L ' %t >/dev/null
