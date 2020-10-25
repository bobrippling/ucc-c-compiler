// ---- leading underscore
//
// RUN:   %ucc -dM -E -xc /dev/null -fleading-underscore    | grep __LEADING_UNDERSCORE
// RUN: ! %ucc -dM -E -xc /dev/null -fno-leading-underscore | grep -i __LEADING_UNDERSCORE
// RUN:   %ucc -dM -E -xc /dev/null -fleading-underscore    | grep '__USER_LABEL_PREFIX__ _'
// RUN: ! %ucc -dM -E -xc /dev/null -fno-leading-underscore | grep -i __USER_LABEL_PREFIX__
//
// ---- pic/pie
//
// -fpic --> __pic__ macros
// RUN:   %ucc -dM -E -xc /dev/null -fpic | grep '__PIC__ 1'
// RUN:   %ucc -dM -E -xc /dev/null -fpic | grep '__pic__ 1'
// RUN:   %ucc -dM -E -xc /dev/null -fPIC | grep '__PIC__ 2'
// RUN:   %ucc -dM -E -xc /dev/null -fPIC | grep '__pic__ 2'
//
// -fpic --> no __pie__ macros
// RUN: ! %ucc -dM -E -xc /dev/null -fpic | grep -i __pie__
// RUN: ! %ucc -dM -E -xc /dev/null -fPIC | grep -i __pie__
//
// -fno-pic --> no __pic__ macros
// RUN: ! %ucc -dM -E -xc /dev/null -fno-pic | grep -i __pic__
//
// -fpie --> __pie__ macros
// RUN:   %ucc -dM -E -xc /dev/null -fpie | grep '__PIE__ 1'
// RUN:   %ucc -dM -E -xc /dev/null -fpie | grep '__pie__ 1'
// RUN:   %ucc -dM -E -xc /dev/null -fPIE | grep '__PIE__ 2'
// RUN:   %ucc -dM -E -xc /dev/null -fPIE | grep '__pie__ 2'
//
// -fpie --> __pic__ macros
// RUN:   %ucc -dM -E -xc /dev/null -fpie | grep '__PIC__ 1'
// RUN:   %ucc -dM -E -xc /dev/null -fpie | grep '__pic__ 1'
// RUN:   %ucc -dM -E -xc /dev/null -fPIE | grep '__PIC__ 2'
// RUN:   %ucc -dM -E -xc /dev/null -fPIE | grep '__pic__ 2'
//
// -fno-pie --> no __pie__ macros
// RUN: ! %ucc -dM -E -xc /dev/null -fno-pie | grep -i __pie__
//
// ---- signed char
//
// RUN: ! %ucc -dM -E -xc /dev/null -fsigned-char      | grep -i __CHAR_UNSIGNED__
// RUN:   %ucc -dM -E -xc /dev/null -fno-signed-char   | grep __CHAR_UNSIGNED__
// RUN: ! %ucc -dM -E -xc /dev/null -fno-unsigned-char | grep -i __CHAR_UNSIGNED__
// RUN:   %ucc -dM -E -xc /dev/null -funsigned-char    | grep __CHAR_UNSIGNED__
//
// ---- stack protector
//
// RUN:   %ucc -dM -E -xc /dev/null -fstack-protector     | grep __SSP__
// RUN: ! %ucc -dM -E -xc /dev/null -fstack-protector     | grep -i __SSP_ALL__
// RUN: ! %ucc -dM -E -xc /dev/null -fstack-protector-all | grep -i __SSP__
// RUN:   %ucc -dM -E -xc /dev/null -fstack-protector-all | grep __SSP_ALL__
//
// ---- fast math
//
// RUN:   %ucc -dM -E -xc /dev/null -fno-fast-math -ffast-math | grep __FAST_MATH__
// RUN: ! %ucc -dM -E -xc /dev/null -ffast-math -fno-fast-math | grep -i __FAST_MATH__
