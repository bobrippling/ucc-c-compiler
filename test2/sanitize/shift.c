// RUN: %ocheck 5 %s -fsanitize=undefined -fsanitize-error=call=san_err -DCALL='toobig_1(sizeof(int) * 8)'
// RUN: %ocheck 5 %s -fsanitize=undefined -fsanitize-error=call=san_err -DCALL='toobig_2(sizeof(int) * 8)'
// RUN: %ocheck 5 %s -fsanitize=undefined -fsanitize-error=call=san_err -DCALL='neg_rhs_1(-1)'
// RUN: %ocheck 5 %s -fsanitize=undefined -fsanitize-error=call=san_err -DCALL='neg_rhs_2(-1)'
// RUN: %ocheck 5 %s -fsanitize=undefined -fsanitize-error=call=san_err -DCALL='neg_lhs(-10)'

int printf(const char *, ...) __attribute((format(printf,1,2)));
void exit(int);

static void san_err(void)
{
	exit(5);
}

toobig_1(int amt)
{
	// amt must be < 8 * sizeof(int)
	return (1 << amt);
}

toobig_2(int amt)
{
	// amt must be < 8 * sizeof(int)
	return (1 >> amt);
}

neg_rhs_1(int i)
{
	return (1 << i);
}

neg_rhs_2(int i)
{
	return (1 >> i);
}

neg_lhs(int i)
{
	return i << 1; // left shift only
}

main()
{
	CALL;

	return 0;
}
