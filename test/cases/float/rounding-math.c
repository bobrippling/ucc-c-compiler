// RUN: ! %ucc -frounding-math    %s -c -o /dev/null
// RUN:   %ucc -fno-rounding-math %s -c -o /dev/null

int x = 3.0 == 3.0;
