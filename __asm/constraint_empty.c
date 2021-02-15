// https://gcc.gnu.org/pipermail/gcc/2021-February/234814.html

int f(int i)
{
	int j = i + 1;
	// empty constraint means `j` must be calculated before this asm, but may be placed anywhere (like "g")
	__asm__("hi %0 %1" : : "r"(i), ""(j));
	return j;
}

// e.g.
// void f(float a, float b)
// {
//   float div = a / b;
//   __asm("" :: ""(div));
//   disable_fpu();
//   use(&div);
//   enable_fpu();
// }
