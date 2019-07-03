unsigned short a[3];

long long foo()
{
  // maybe a bug
  return (a[1] << 16) | (a[2]+0ull<<32);
}
