enum E
{
	A,
	B = A
};

main()
{
	enum {
		A = A
	};
	return A;
}
