// RUN: %check -e %s

int j;
long l = &j; // fine

int i = &j; // CHECK: /error:.*not constant/
