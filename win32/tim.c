enum { MB_OK = 0 };

// force the calling convention to be stack-only
// note that it's callee cleanup - need to hand edit for this
extern int _MessageBoxA(long, ...);

main()
{
	char buf[8];
	int i;

	buf[0] = 'h';
	buf[1] = 'i';
	buf[2] = ' ';
	buf[4] = 0;

	for(i = 0; i < 10; i++){
		buf[3] = i + '0';

		_MessageBoxA(0, buf, "Caption", MB_OK);
	}

	return 5;
}
