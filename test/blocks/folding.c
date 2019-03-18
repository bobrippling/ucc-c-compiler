// RUN: %ocheck 1 %s

typedef void *Ptr;
typedef int Integer;
typedef Integer ComparisonResult;
typedef ComparisonResult (^Comparator)(Ptr obj1, Ptr obj2);

main()
{
	Ptr a, b;
	Comparator x = ^(Ptr arg_a, Ptr arg_b){
		return arg_a == arg_b;
	};

	b = (a = (void *)2) + 1 - 1;

	return x(a, b);
}
