typedef struct {
    unsigned char a;
    unsigned char b;
    unsigned short c;
    unsigned int d;
    unsigned long e;
} __attribute__ ((packed)) s;

struct t
{
  long long x;  // Force 64-bit alignment
  char y;       // One byte into next double-word
  s z;          // Amount of padding before this will show alignment needed.
};

unsigned u = __builtin_offsetof (struct t, z);
