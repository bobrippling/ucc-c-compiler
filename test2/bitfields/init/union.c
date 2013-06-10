// RUN: %ucc -o %t %s
// RUN: %t

union
{
	int i;
	signed bf : 1;
} x = {
	.bf = 5 // 1 TODO: warn here
};

union
{
	int x : 2;
} tim = {
	4 // zero XXX: TODO: warn here
};

main()
{
	if(x.bf != -1)
		return 1;
	if(tim.x != 0)
		return 1;
	return 0;
}
