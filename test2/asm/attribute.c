// RUN: %ucc -fsyntax-only %s

int x __asm("y") __attribute__((unused));
