// RUN: %ucc -o %t %s
// RUN: %t

main()
{
  int a[4];
  return 3[a];
}
