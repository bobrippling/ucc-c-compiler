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
