// RUN: %check -e %s

void (*^blk)(); // CHECK: error: invalid block pointer - function required
