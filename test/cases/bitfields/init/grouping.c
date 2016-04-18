// RUN: %ucc -o %t %s
// RUN: %t | %output_check '0x18200000001'
struct Padded
{
	int i : 2;
	int : 0;
	int j : 3;
	int : 4;
	int end : 13;
};

struct Padded pad = {
	1, 2, 3 // should initialise i, j and k, skipping unnamed fields
};

main()
{
	int printf();
	printf("0x%lx\n", *(long *)&pad);
}
