// RUN: ! %ucc     -dM -E -xc /dev/null | grep __OPTIMIZE__
// RUN:   %ucc -O  -dM -E -xc /dev/null | grep __OPTIMIZE__
// RUN: ! %ucc -O  -dM -E -xc /dev/null | grep __OPTIMIZE_SIZE__
// RUN:   %ucc -Os -dM -E -xc /dev/null | grep __OPTIMIZE_SIZE__
