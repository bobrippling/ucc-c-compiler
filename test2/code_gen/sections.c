// RUN: %layout_check --sections %s -fno-common -target x86_64-linux

// rodata
const main = 195;

// bss
int x;

// data
int y = 1;

// text
const int f()
{
}
