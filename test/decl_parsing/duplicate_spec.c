// RUN: %check %s

volatile volatile volatile int i; // CHECK: warning: duplicate 'volatile' specifier
