// RUN: %check -e %s
int y;
float x = (float)&y; // CHECK: /error: cast from pointer to floating type/
