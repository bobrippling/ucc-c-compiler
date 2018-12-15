// RUN: %ocheck 0 %s

main()
{
  int a[4] = { 0 };
  return 3[a];
}
