enum { MB_OK, MB_OK_CANCEL };

// note that it's callee cleanup - need to hand edit for this
extern __attribute__((stdcall)) int
_MessageBoxA(int, char *, char *, int)
;//asm("_MessageBoxA@16");

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

		// this currently calls the _cdecl impl, so we aren't doing stack clean that we should
		_MessageBoxA(0, buf, "Caption", MB_YES_CANCEL);
	}

	return 5;
}
