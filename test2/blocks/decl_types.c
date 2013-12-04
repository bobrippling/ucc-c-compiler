// RUN: %ocheck 10 %s

main()
{
	int i = 1;

	return
		^int (int i){ return i; }(i) +
		^(int i){ return i; }(i + 1) +
		^int { return 3; }() +
		^{ return 4; }();
}
