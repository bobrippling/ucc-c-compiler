--- gcc/testsuite/gcc.dg/c90-init-1.c.jj	Thu Dec  7 14:17:42 2000
+++ gcc/testsuite/gcc.dg/c90-init-1.c	Thu Dec  7 14:40:36 2000
@@ -0,0 +1,25 @@
+/* Test for C99 designated initializers */
+/* Origin: Jakub Jelinek &lt;jakub@redhat.com&gt; */
+/* { dg-do compile } */
+/* { dg-options "-std=iso9899:1990 -pedantic-errors" } */
+
+struct A {
+  int B;
+  short C[2];
+};
+int a[10] = { 10, [4] = 15 };			/* { dg-error "ISO C89 forbids specifying subobject to initialize" } */
+struct A b = { .B = 2 };			/* { dg-error "ISO C89 forbids specifying subobject to initialize" } */
+struct A c[] = { [3].C[1] = 1 };		/* { dg-error "ISO C89 forbids specifying subobject to initialize" } */
+struct A d[] = { [4 ... 6].C[0 ... 1] = 2 };	/* { dg-error "(forbids specifying range of elements to initialize)|(ISO C89 forbids specifying subobject to initialize)" } */
+int e[] = { [2] 2 };				/* { dg-error "use of designated initializer without" } */
+struct A f = { C: { 0, 1 } };			/* { dg-error "use of designated initialize.r with " } */
+int g;
+
+void foo (int *);
+
+void bar (void)
+{
+  int x[] = { g++, 2 };				/* { dg-error "is not computable at load time" } */
+
+  foo (x);
+}
--- gcc/testsuite/gcc.dg/c99-init-1.c.jj	Thu Dec  7 11:04:38 2000
+++ gcc/testsuite/gcc.dg/c99-init-1.c	Thu Dec  7 14:18:19 2000
@@ -0,0 +1,78 @@
+/* Test for C99 designated initializers */
+/* Origin: Jakub Jelinek &lt;jakub@redhat.com&gt; */
+/* { dg-do run } */
+/* { dg-options "-std=iso9899:1999 -pedantic-errors" } */
+
+typedef __SIZE_TYPE__ size_t;
+typedef __WCHAR_TYPE__ wchar_t;
+extern int memcmp (const void *, const void *, size_t);
+extern void abort (void);
+extern void exit (int);
+
+int a[10] = { 10, 0, 12, 13, 14, 0, 0, 17, 0, 0 };
+int b[10] = { 10, [4] = 15, [2] = 12, [4] = 14, [7] = 17 };
+int c[10] = { 10, [4] = 15, [2] = 12, [3] = 13, 14, [7] = 17 };
+struct A {
+  int B;
+  short C[2];
+};
+struct A d[] = { { 0, { 1, 2 } }, { 0, { 0, 0 } }, { 10, { 11, 12 } } };
+struct A e[] = { 0, 1, 2, [2] = 10, 11, 12 };
+struct A f[] = { 0, 1, 2, [2].C = 11, 12, 13 };
+struct A g[] = { 0, 1, 2, [2].C[1] = 12, 13, 14 };
+struct A h[] = { 0, 1, 2, [2] = { .C[1] = 12 }, 13, 14 };
+struct A i[] = { 0, 1, 2, [2] = { .C = { [1] = 12 } }, 13, 14 };
+union D {
+  int E;
+  double F;
+  struct A G;
+};
+union D j[] = { [4] = 1, [4].F = 1.0, [1].G.C[1] = 4 };
+struct H. {
+  char I[6];
+  int J;
+} k[] = { { { "foo" }, 1 }, [0].I[0] = 'b' };
+struct K {
+  wchar_t L[6];
+  int M;
+} l[] = { { { L"foo" }, 1 }, [0].L[2] = L'x', [0].L[4] = L'y' };
+struct H m[] = { { { "foo" }, 1 }, [0] = { .I[0] = 'b' } };
+struct H n[] = { { { "foo" }, 1 }, [0].I = { "a" }, [0].J = 2 };
+int o = { 22 };
+
+int main (void)
+{
+  if (b[3])
+    abort ();
+  b[3] = 13;
+  if (memcmp (a, b, sizeof (a)) || memcmp (a, c, sizeof (a)))
+    abort ();
+  if (memcmp (d, e, sizeof (d)) || sizeof (d) != sizeof (e))
+    abort ();
+  if (f[2].B != 0 || g[2].B != 0 || g[2].C[0] != 0)
+    abort ();
+  if (memcmp (g, h, sizeof (g)) || memcmp (g, i, sizeof (g)))
+    abort ();
+  f[2].B = 10;
+  g[2].B = 10;
+  g[2].C[0] = 11;
+  if (memcmp (d, f, sizeof (d)) || memcmp (d, g, sizeof (d)))
+    abort ();
+  if (f[3].B != 13 || g[3].B != 13 || g[3].C[0] != 14)
+    abort ();
+  if (j[0].E || j[1].G.B || j[1].G.C[0] || j[1].G.C[1] != 4)
+    abort ();
+  if (j[2].E || j[3].E || j[4].F != 1.0)
+    abort ();
+  if (memcmp (k[0].I, "boo\0\0", 6) || k[0].J != 1)
+    abort ();
+  if (memcmp (l[0].L, L"fox\0y", 6 * sizeof(wchar_t)) || l[0].M != 1)
+    abort ();
+  if (memcmp (m[0].I, "b\0\0\0\0", 6) || m[0].J)
+    abort ();
+  if (memcmp (n[0].I, "a\0\0\0\0", 6) || n[0].J != 2)
+    abort ();
+  if (o != 22)
+    abort ();
+  exi..t (0);
+}
--- gcc/testsuite/gcc.dg/c99-init-2.c.jj	Thu Dec  7 11:04:38 2000
+++ gcc/testsuite/gcc.dg/c99-init-2.c	Thu Dec  7 15:03:14 2000
@@ -0,0 +1,30 @@
+/* Test for C99 designated initializer warnings and errors */
+/* Origin: Jakub Jelinek &lt;jakub@redhat.com&gt; */
+/* { dg-do compile } */
+/* { dg-options "-std=iso9899:1999 -Wall -pedantic-errors" } */
+
+typedef struct {
+  int B;
+  short C[2];
+} A;
+A a = { [2] = 1 };			/* { dg-error "(array index in non-array)|(near initialization)" } */
+int b[] = { .B = 1 };			/* { dg-error "(field name not in record)|(near initialization)" } */
+A c[] = { [0].D = 1 };			/* { dg-error "unknown field" } */
+int d;
+int e = { d++ };			/* { dg-error "(is not constant)|(near initialization)" } */
+A f[2] = { [0].C[0] = 1, [2] = { 2, { 1, 2 } } };/* { dg-error "(array index in initializer exceeds array bounds)|(near initialization)" } */
+int g[4] = { [1] = 1, 2, [6] = 5 };	/* { dg-error "(array index in initializer exceeds array bounds)|(near initialization)" } */
+int h[] = { [0 ... 3] = 5 };		/* { dg-error "forbids specifying range of elements" } */
+int i[] = { [2] 4 };			/* { dg-error "use of designated initializer without" } */
+A j = { B: 2 };				/* { dg-error "use of designated initializer with " } */
+
+void foo (int *, A *);
+
+void bar (void)
+{
+  int a[] = { d++, [0] = 1. };		/* { dg-warning "(initialized field with side-effects overwritten)|(near initialization)" } */
+  A b = { 1, { d++, 2 }, .C[0] = 3 };/* { dg-warning "(initialized field with side-effects overwritten)|(near initialization)" } */
+  A c = { d++, { 2, 3 }, .B = 4 };	/* { dg-warning "(initialized field with side-effects overwritten)|(near initialization)" } */
+
+  foo (a, d ? &amp;b : &amp;c);
+}
--- gcc/testsuite/gcc.dg/gnu99-init-1.c.jj	Thu Dec  7 11:04:38 2000
+++ gcc/testsuite/gcc.dg/gnu99-init-1.c	Thu Dec  7 14:16:29 2000
@@ -0,0 +1,62 @@
+/* Test for GNU extensions to C99 designated initializers */
+/* Origin: Jakub Jelinek &lt;jakub@redhat.com&gt; */
+/* { dg-do run } */
+/* { dg-options "-std=gnu99" } */
+
+typedef __SIZE_TYPE__ size_t;
+extern int memcmp (const void *, const void *, size_t);
+extern void abort (void);
+extern void exit (int);
+
+int a[][2][4] = { [2 ... 4][0 ... 1][2 ... 3] = 1, [2] = 2, [2][0][2] = 3 };
+struct E {};
+struct F { struct E H; };
+struct G { int I; struct E J; int K; };
+struct H { int I; struct F J; int K; };
+struct G k = { .J = {}, 1 };
+struct H l = { .J.H = {}, 2 };
+struct H m = { .J = {}, 3 };
+struct I { int J; int K[3]; int L; };
+struct M { int N; struct I O[3]; int P; };
+struct M n[] = { [0 ... 5].O[1 ... 2].K[0 ... 1] = 4, 5, 6, 7 };
+
+int main (void)
+{
+  int x, y, z;
.+
+  if (a[2][0][0] != 2 || a[2][0][2] != 3)
+    abort ();
+  a[2][0][0] = 0;
+  a[2][0][2] = 1;
+  for (x = 0; x &lt;= 4; x++)
+    for (y = 0; y &lt;= 1; y++)
+      for (z = 0; z &lt;= 3; z++)
+	if (a[x][y][z] != (x &gt;= 2 &amp;&amp; z &gt;= 2))
+	  abort ();
+  if (k.I || l.I || m.I || k.K != 1 || l.K != 2 || m.K != 3)
+    abort ();
+  for (x = 0; x &lt;= 5; x++)
+    {
+      if (n[x].N || n[x].O[0].J || n[x].O[0].L)
+	abort ();
+      for (y = 0; y &lt;= 2; y++)
+	if (n[x].O[0].K[y])
+	  abort ();
+      for (y = 1; y &lt;= 2; y++)
+	{
+	  if (n[x].O[y].J)
+	    abort ();
+	  if (n[x].O[y].K[0] != 4)
+	    abort ();
+	  if (n[x].O[y].K[1] != 4)
+	    abort ();
+	  if ((x &lt; 5 || y &lt; 2) &amp;&amp; (n[x].O[y].K[2] || n[x].O[y].L))
+	    abort ();
+	}
+      if (x &lt; 5 &amp;&amp; n[x].P)
+	abort ();
+    }
+  if (n[5].O[2].K[2] != 5 || n[5].O[2].L != 6 || n[5].P != 7)
+    abort ();
+  exit (0);
+}
