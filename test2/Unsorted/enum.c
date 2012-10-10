enum B
{
	X, Y, Z
};

main()
{
	enum { A, B, C, D, E, F, } a = B;
	enum B b = X;

	switch(a){
		case B ... D:;
	}

	b == a; // warn

	return Z;
}
