// RUN: %ucc -fsyntax-only %s

volatile const x;

typedef const int kint;

volatile kint x;
kint volatile x;

// should all match / compare equal
