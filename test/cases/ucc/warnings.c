// RUN: %ucc -Wall -Wno-extra -Weverything -Wgnu -Werror -Wcpp -Wpaste -Wno-traditional -Wunused -Wno-implicit -Werror=overflow -Wno-error=vla -Werror=everything %s -'###' >%t 2>&1

// true and false groups:
// -Wall
// -Wno-extra
// -Weverything
// -Wgnu
// -Werror

// non existent option:
// -Wcpp

// true and false cpp options:
// -Wpaste
// -Wno-traditional

// true, false and Werror cc1 options:
// -Wunused
// -Wno-implicit
// -Werror=overflow
// -Wno-error=vla
// -Werror=everything // this is special because it's a pseudo warning inside -Werror

// RUN:   grep -q "/cpp .*-Wall" %t
// RUN: ! grep -q "/cpp .*-Wno-extra" %t
// RUN:   grep -q "/cpp .*-Weverything" %t
// RUN: ! grep -q "/cpp .*-Wgnu" %t
// RUN: ! grep -q "/cpp .*-Werror " %t
// RUN: ! grep -q "/cpp .*-Wcpp" %t
// RUN:   grep -q "/cpp .*-Wpaste" %t
// RUN:   grep -q "/cpp .*-Wno-traditional" %t
// RUN: ! grep -q "/cpp .*-Wunused" %t
// RUN: ! grep -q "/cpp .*-Wno-implicit" %t
// RUN: ! grep -q "/cpp .*-Werror=overflow" %t
// RUN: ! grep -q "/cpp .*-Wno-error=vla" %t
// RUN: ! grep -q "/cpp .*-Werror=everything" %t

// RUN:   grep -q "/cc1 .*-Wall" %t
// RUN:   grep -q "/cc1 .*-Wno-extra" %t
// RUN:   grep -q "/cc1 .*-Weverything" %t
// RUN:   grep -q "/cc1 .*-Wgnu" %t
// RUN:   grep -q "/cc1 .*-Werror " %t
// RUN:   grep -q "/cc1 .*-Wcpp" %t
// RUN: ! grep -q "/cc1 .*-Wpaste" %t
// RUN: ! grep -q "/cc1 .*-Wno-traditional" %t
// RUN:   grep -q "/cc1 .*-Wunused" %t
// RUN:   grep -q "/cc1 .*-Wno-implicit" %t
// RUN:   grep -q "/cc1 .*-Werror=overflow" %t
// RUN:   grep -q "/cc1 .*-Wno-error=vla" %t
// RUN:   grep -q "/cc1 .*-Werror=everything" %t
