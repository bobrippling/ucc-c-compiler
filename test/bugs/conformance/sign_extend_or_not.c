unsigned short a[3];

long long foo()
{
  // a[2] gets converted to int, then to unsigned long long via sign-extension. bug?
  return (a[1] << 16) | (a[2]+0ull<<32);
}
