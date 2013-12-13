// RUN: %ucc -c %s

main()
{
	f(-(1LL << 35)); // should emit a movabsq for this
}
