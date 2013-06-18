// RUN: %ucc -o %t %s
// RUN: %t | %output_check '(nil) 5' '(nil) 6' '(nil) 7'
#define NULL (void *)0

static struct
{
	long *p;
	int i;
	// pad 4
} tims[] = {
	NULL, 5,
	NULL, 6,
	NULL, 7,
};

main()
{
	for(int i = 0; i < 3; i++)
		printf("%p %d\n", tims[i].p, tims[i].i);
}
