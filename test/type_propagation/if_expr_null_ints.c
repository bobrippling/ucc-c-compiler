// RUN: %ucc -fsyntax-only %s

__auto_type x = 3 ? (void*)0 : 0;
