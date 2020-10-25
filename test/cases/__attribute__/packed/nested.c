// RUN: %layout_check %s

struct unpacked
{
  char c;
  short s;
};

struct __attribute__((packed)) packed
{
  char c;
  short  s;
  struct unpacked sub;
};

struct __attribute__((packed)) packed2
{
  char c;
  short  s;
  struct unpacked sub __attribute__((packed));
};

struct packed a = {
  .c = 10,
  .s = 20,
  .sub.c = 30,
  .sub.s = 40
};

struct packed2 b = {
  .c = 10,
  .s = 20,
  .sub.c = 30,
  .sub.s = 40
};
