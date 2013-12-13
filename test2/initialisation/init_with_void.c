// RUN: %check -e %s
int i = (void)5; // CHECK: /error: initialisation from void expression/
