// RUN: %check -e %s

f()
{
  l:

  {
    __label__ l;
l:; // CHECK: !/error/
  }

  {
l:; // CHECK: error: duplicate label 'l'
  }
}
