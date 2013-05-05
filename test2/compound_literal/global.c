// RUN: %check -e %s

int i = (int){1}; // CHECK: /error: .*not constant/
int j[] = (int[]){ 1, 2 };
