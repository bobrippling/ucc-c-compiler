// RUN: %ucc -target x86_64-linux -S -o %t %s -fcommon
// RUN:   grep -F '.weak a1' %t
// RUN: ! grep -F '.weak b1' %t
// RUN:   grep -F '.weak a2' %t
// RUN:   grep -F '.weak b2' %t
// RUN:   grep -F 'b1 = a1' %t
// RUN:   grep -F 'b2 = a2' %t
// not emitted as weak:
// RUN: ! grep -F '.comm a1,4,4' %t
// RUN:   grep -F 'a1:' %t
// RUN: ! grep -F '.comm a2,4,4' %t
// RUN:   grep -F 'a2:' %t
//
// RUN: %ucc -target x86_64-linux -S -o %t %s -fno-common
// RUN:   grep -F '.weak a1' %t
// RUN: ! grep -F '.weak b1' %t
// RUN:   grep -F '.weak a2' %t
// RUN:   grep -F '.weak b2' %t
// RUN:   grep -F 'b1 = a1' %t
// RUN:   grep -F 'b2 = a2' %t
// RUN: ! grep -F '.comm a1,4,4' %t
// RUN:   grep -F 'a1:' %t
// RUN: ! grep -F '.comm a2,4,4' %t
// RUN:   grep -F 'a2:' %t

__attribute((weak)) int a1; /* 'a1' is a candidate for a common symbol, but this doesn't combine with being weak */
__attribute((alias("a1"))) extern int b1;

__attribute((weak)) int a2;
__attribute((weak, alias("a2"))) extern int b2;
