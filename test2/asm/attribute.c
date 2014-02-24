// RUN: %ucc -fsyntax-only %s

int x asm("y") __attribute__((unused));
