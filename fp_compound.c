main()
{
	float a = 0;
	int i = 0;

	a += 1; // 1 cast to float
	i += (float)1; // i loaded, cast to float, added, cast to int and stored

	a = i++; // i -> float, stored to a. i += (int)1
	i = a++; // a loaded, converted to int, stored to i.
	         // a loaded, added to (float)1, stored
}
