main(int argc)
{
  void *labels[] = {
    &&a, &&b, &&c,
  };
  int i;

  goto *labels[argc - 1];

  a: i = 1; goto fin;
  b: i = 2; goto fin;
  c: i = 3; goto fin;

fin:
  return i;
}
