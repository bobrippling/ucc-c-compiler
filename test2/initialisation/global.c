#define INIT(...) (int[]){ __VA_ARGS__ }

int y[] = INIT( 5, 6 );

int (*p)[] = &INIT(3, 4);

main()
{
	static int x[] = INIT( 1, 2 );
}
