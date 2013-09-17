// RUN: %ucc -o %t %s
// RUN: %t | %output_check 'nan'

f(float f)
{
	printf("%f\n", f);
}

main()
{
	f(__builtin_nanf(""));
}
