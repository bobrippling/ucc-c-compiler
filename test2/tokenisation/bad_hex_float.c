// RUN: %check -e %s

float b1 = 0xp1; // CHECK: error: invalid suffix on floating point constant (x)
float b2 = 0x1p; // CHECK: error: invalid suffix on floating point constant (p)
float b3 = 0xdE.488631pA; // CHECK: error: invalid suffix on floating point constant (p)
