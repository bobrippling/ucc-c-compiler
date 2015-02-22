// RUN: %check -e %s

float b1 = 0xp1; // CHECK: error: invalid suffix on integer constant (x)
float b2 = 0x1p; // CHECK: error: invalid suffix on integer constant (p)
