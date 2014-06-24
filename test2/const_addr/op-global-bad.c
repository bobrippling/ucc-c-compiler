// RUN: %check -e %s

int glob;

int addr_cmp1 = 1 == &glob; // CHECK: error: global scalar initialiser not constant
int addr_cmp2 = 1 > &glob; // CHECK: error: global scalar initialiser not constant

int *q = glob + 2; // CHECK: error: global scalar initialiser not constant

int sum1 = +&glob; // CHECK: error: global scalar initialiser not constant
int sum2 = -&glob; // CHECK: error: global scalar initialiser not constant

