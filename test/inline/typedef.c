// RUN: %check -e %s

typedef inline int ifn(void); // CHECK: error: typedef has inline specified

ifn a;

a()
{
	return 3;
}
