main()
{
#define ALIGN(t, s) printf("align of " #t " = " s "\n", __alignof__(t))

	ALIGN(int, "%d");
	ALIGN(unsigned int, "%d");
	ALIGN(short, "%d");
	ALIGN(unsigned short, "%d");
	ALIGN(char, "%d");
	ALIGN(unsigned char, "%d");
	ALIGN(func, "%d");
}
