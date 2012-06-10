#define NULL (void *)0

void _start(void)
{
	extern void _ExitProcess(int);
	extern int main(int, char **);

	_ExitProcess(main(0, NULL));
}
