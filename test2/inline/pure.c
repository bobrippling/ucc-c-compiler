// RUN: %check %s
// RUN: %ocheck 3 %s

inline int notemitted() // CHECK: warning: pure inline function will not have code emitted (missing "static" or "extern")
{
	return 2;
}

inline int yo() // CHECK: !/warn/
{ // CHECK: !/warn/
	return 3;
}

static int yo(); // CHECK: !/warn/

main()
{
	return yo();
}
