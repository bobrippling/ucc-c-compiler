// RUN: %layout_check --sections %s

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
