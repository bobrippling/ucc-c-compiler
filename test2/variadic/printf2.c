// RUN: %ucc -o %t %s
// RUN: %t | %output_check 'hi 3, yo'

strlen2(char *s)
{
	int i = 0;
	for(; *s; s++, i++);
	return i;
}

printf2(char *fmt, ...)
{
	__builtin_va_list l;

	__builtin_va_start(l, 2531);

	for(; *fmt; fmt++)
		if(*fmt == '%'){
			switch(*++fmt){
				case 's':;
					char *s = __builtin_va_arg(l, char *);
					write(1, s, strlen2(s));
					break;
				case 'd':;
					int d = '0' + __builtin_va_arg(l, int);
					write(1, &d, 1); // assume char
					break;
				default:
					write(1, fmt-1, 1); // '%'
					goto norm;
			}
		}else{
norm:
			write(1, fmt, 1);
		}

	__builtin_va_end(l);
}

main()
{
	printf2("hi %d, %s\n", 3, (char[]){'y', 'o', 0});
}
