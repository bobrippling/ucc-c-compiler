struct Hello;

//enum Hello { X };

struct Hello
{
	int i, j;
};

main()
{
	struct Hello x;

	x.j = 5;

	return (&x)->j;
}
