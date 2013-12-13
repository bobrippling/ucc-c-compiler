// RUN: %check -e %s

void *x = &&yo; // CHECK: /error: address-of-label outside a function/
