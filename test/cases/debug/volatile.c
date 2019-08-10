// RUN: %ucc -g -S -o- %s | grep DW_TAG_volatile_type >/dev/null
volatile int *p = 0xffff;
