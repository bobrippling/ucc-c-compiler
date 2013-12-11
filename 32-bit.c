typedef long long ll;

ll add(ll a, ll b)
{
	return a + b;
}

ll saturate(
		ll a1, ll a2, ll a3,
		ll a4, ll a5, ll a6)
{
	return a1 + a2 + a3 + a4 + a5 + a6;
}

double to_dbl(ll a)
{
	return a;
}

ll from_dbl(double d)
{
	return d;
}

ll sign_ext(int i)
{
	return i;
}

unsigned long long zero_ext(int i)
{
	return i;
}

main()
{
	if(add(1, 2) != 3)
		abort();
	if(saturate(1, 2, 3, 4, 5, 6) != 21)
		abort();
	if(to_dbl(3251) != 3251)
		abort();
	if(from_dbl(156) != 156)
		abort();
	if(sign_ext(-5) != -5)
		abort();
	if(zero_ext(923546) != 923546)
		abort();
	return 0;
}
