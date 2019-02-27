// RUN: ! %ucc -E -xc /dev/null -dM         | grep '__OPTIMIZE__'
// RUN: ! %ucc -E -xc /dev/null -dM         | grep '__OPTIMIZE_SIZE__'
//
// RUN:   %ucc -E -xc /dev/null -dM -O1     | grep '__OPTIMIZE__'
// RUN: ! %ucc -E -xc /dev/null -dM -O1     | grep '__OPTIMIZE_SIZE__'
//
// RUN:   %ucc -E -xc /dev/null -dM -Os     | grep '__OPTIMIZE__'
// RUN:   %ucc -E -xc /dev/null -dM -Os     | grep '__OPTIMIZE_SIZE__'
//
// RUN: ! %ucc -E -xc /dev/null -dM -Os -O0 | grep '__OPTIMIZE__'
// RUN: ! %ucc -E -xc /dev/null -dM -Os -O0 | grep '__OPTIMIZE_SIZE__'
//
// RUN: ! %ucc -E -xc /dev/null -dM -O2 -O0 | grep '__OPTIMIZE__'
// RUN: ! %ucc -E -xc /dev/null -dM -O2 -O0 | grep '__OPTIMIZE_SIZE__'
