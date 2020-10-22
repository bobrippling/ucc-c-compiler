// RUN: [ `%ucc -c %s 2>&1 | grep 'warning: index 6 out of bounds' | wc -l` -eq 1 ]
// RUN: %check %s

main()
{
	int x[5];

	return x[6]; // CHECK: /warning: index 6 out of bounds of array, size 5/
}
