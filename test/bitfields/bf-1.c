// RUN: %ocheck 0 %s

struct single_signed
{
	int bf : 1;
};

main()
{
	struct single_signed a;

	a.bf = 1;

	if(a.bf != -1)
		return 1;

	return 0;
}
