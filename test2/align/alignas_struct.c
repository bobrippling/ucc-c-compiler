// RUN: %ucc -c %s
// RUN: %ucc -S -o- %s | asmcheck %s

struct align
{
	char pre;
	char _Alignas(16) bet;
	char post;
} glob = { 1, 2, 3 };

struct align al;

stru()
{
	al.pre = 1;
	al.bet = 2;
	al.post = 3;
}
