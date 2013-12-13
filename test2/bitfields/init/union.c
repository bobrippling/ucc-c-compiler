// RUN: %ucc -o %t %s
// RUN: %t
// RUN: %check %s

union
{
	int i;
	signed bf : 1;
} x = {
	.bf = 5 // CHECK: /warning: truncation in store to bitfield alters value: 5 -> 1/
};

union
{
	int x : 2;
} tim = {
	4 // CHECK: /warning: truncation in store to bitfield alters value: 4 -> 0/
};

main()
{
	if(x.bf != -1)
		return 1;
	if(tim.x != 0)
		return 1;
	return 0;
}
