assignment()
{
	pa = &a, pb = &b; // should go to (pa = &a), (pb = &b);

	x == *p = *a; // should go to (x == *p) = *a
}
