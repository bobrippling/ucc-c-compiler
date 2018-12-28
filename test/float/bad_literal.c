// RUN: %check -e %s

float g = 0.2; // CHECK: !/error/

float f = 0x.2; // CHECK: error: floating literal requires exponent
float h = 0b.2; // CHECK: error: invalid prefix on floating literal
