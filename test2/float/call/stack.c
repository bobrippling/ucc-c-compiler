// RUN: %ucc -DLIB -c -o %t.lib.o %s
// RUN: %ucc       -c -o %t.prg.o %s
// RUN: %ucc -o %t %t.prg.o %t.lib.o
// RUN: %t | %output_check '1.00 2.00 3.00 4.00 5.00 6.00 7.00 8.00 9.00'

g(float a, float b, float c, float d,
  float e, float f, float g, float h,
  float i);

#ifdef LIB
g(float a, float b, float c, float d,
  float e, float f, float g, float h,
  float i)
{
	printf("%.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f\n",
			a, b, c, d, e, f, g, h, i);
}
#else
main()
{
	g(1, 2, 3, 4, 5, 6, 7, 8, 9);
}
#endif
