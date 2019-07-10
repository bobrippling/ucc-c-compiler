// RUN: %ucc -o %t %s
// RUN: %t | grep -- -
long write(int fildes, const void *buf, unsigned long nbyte);
int printf(const char *, ...) __attribute__((format(printf, 1, 2)));

main()
{
	if(1){
		int neg = '-';
		write(1, &neg, 1);
	}
	printf("\n");

	return 0;
}
