// RUN: %ucc -o %t %s
// RUN: %t | %output_check '0x0 5' '0x0 6' '0x0 7'
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

int printf();

main()
{
	for(int i = 0; i < 3; i++)
		printf("0x%x %d\n", (long)tims[i].p, tims[i].i);
}
