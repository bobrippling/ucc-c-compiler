main()
{
	// FIXME: code gen / const-folding shifts
	f(1    << 35); // warn
	f(1U   << 35); // warn
	f(1L   << 35); // no warn
	f(1LU  << 35); // no warn
	f(1ULL << 35); // no warn
}
