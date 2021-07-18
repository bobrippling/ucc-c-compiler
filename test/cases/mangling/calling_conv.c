// RUN: %ucc %s -o %t
// RUN: %t | %output_check 36 36 36

cdecl(a, b, c, d, e, f, g, h) __attribute__((cdecl))
{
	return a + b + c + d + e + f + g + h;
}

stdcall(a, b, c, d, e, f, g, h) __attribute__((stdcall))
{
	return a + b + c + d + e + f + g + h;
}

fastcall(a, b, c, d, e, f, g, h) __attribute__((fastcall))
{
	return a + b + c + d + e + f + g + h;
}

main()
{
	printf("%d\n",
			cdecl(1, 2, 3, 4, 5, 6, 7, 8));

	printf("%d\n",
			stdcall(1, 2, 3, 4, 5, 6, 7, 8));

	printf("%d\n",
			fastcall(1, 2, 3, 4, 5, 6, 7, 8));

	// check names are correct
	__asm("call _cdecl");
	__asm("call _stdcall@32");
	__asm("call @fastcall@32");

	return 0;
}
