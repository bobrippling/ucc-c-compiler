// RUN: %ucc -c %s

void f(long long);

main()
{
	f(-(1LL << 35)); // should emit a movabsq for this
}
