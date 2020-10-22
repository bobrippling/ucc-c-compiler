// RUN: %ucc -E %s -o %t

#define NULL ((void *)0)

main()
{
	int b = parse_int(rest+1, NULL'\0');
}
