// RUN: %ocheck 0 %s

char *oct = "a\0563";
char *hexterm = "\xagh";
char *s = "h\xbcdef515abd\03321\012345";
//         h\275         \03321\n  345
// 104, 189/-67, 27, 50, 49, 10, 51, 52, 53, 0

main()
{
	if(strcmp(oct, "a.3"))
		abort();

	if(strcmp(hexterm, (char[]){ 10, 'g', 'h', 0 }))
		abort();

	if(strcmp(s, "h\275\03321\n345"))
		abort();

	return 0;
}
