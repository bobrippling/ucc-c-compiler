// RUN: %ucc -S -o- %s | %check %s

f(int x[])
{
	int a[2];
	return
		sizeof x + // WARN: /warning: array parameter size is sizeof\(int \*\), not sizeof\(int \[\]\)/
		sizeof(int) +
		sizeof a;
}
