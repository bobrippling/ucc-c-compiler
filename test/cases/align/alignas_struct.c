// RUN: %ucc -c %s
// RUN: %layout_check %s

struct align
{
	char pre;
	char _Alignas(16) bet;
	char post;
} glob = {
	1, 2, 3
};

struct align_substruct
{
	char pre;
	_Alignas(16) struct sub
	{
		char a, b, c;
	} bet;
	char post;
} glob_s = {
	1, { 2, 3, 4 }, 5
};
