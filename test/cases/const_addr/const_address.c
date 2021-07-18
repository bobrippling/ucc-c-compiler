// RUN: %ucc -c %s
// RUN: %ucc %s -S -o- | grep -F 's + 1296' > /dev/null

struct RT
{
  char A; // 1
  int B[10][20]; // 20*10*4 = 800
  char C; // 1
}; // sz = 1 + 3 + 800 + 1 + 3 = 808

struct ST
{
  int X; // 4
  double Y; // 8
  struct RT Z; // 808
} s[2]; // sz = 4 + 4 + 8 + 808 = 824

int *p = &s[1].Z.B[5][13];
