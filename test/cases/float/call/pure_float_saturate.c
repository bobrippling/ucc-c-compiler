// RUN: %ocheck 0 %s

void abort(void);

f(float a,
  float b,
  float c,
  float d,
  float e,
  float f,
	float g,
	float h,
	float i)
{
#define check(x, n) if(x != n) abort()

	check(a, 1);
	check(b, 2);
	check(c, 3);
	check(d, 4);
	check(e, 5);
	check(f, 6);
	check(g, 7);
	check(h, 8);
	check(i, 9);
}

main()
{
	f(1,2,3,4,5,6,7,8,9);
}
