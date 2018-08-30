// RUN: %ucc -fsyntax-only %s

int i = (int){1};
int j[] = (int[]){ 1, 2 };
int k[] = (int[3]){ 1 };
