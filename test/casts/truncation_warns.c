// RUN: %check --only %s -emit=dump
// note: shouldn't need -Wconstant-conversion - on by default
// use -emit=dump to avoid ICW for large movabs

typedef int sint;
typedef unsigned uint;

main()
{
	__attribute__((unused)) int      a1 = -1UL; // CHECK: /warning: implicit cast negates value, 18446744073709551615 to -1/
	__attribute__((unused)) int      b1 = ~0UL; // CHECK: /warning: implicit cast negates value, 18446744073709551615 to -1/
	__attribute__((unused)) unsigned c1 = -1UL; // CHECK: /warning: implicit cast truncates value from 18446744073709551615 to 4294967295/
	__attribute__((unused)) unsigned d1 = ~0UL; // CHECK: /warning: implicit cast truncates value from 18446744073709551615 to 4294967295/

	__attribute__((unused)) long int      a2 = -1UL; // no warnings: long --> long
	__attribute__((unused)) long int      b2 = ~0UL;
	__attribute__((unused)) long unsigned c2 = -1UL;
	__attribute__((unused)) long unsigned d2 = ~0UL;
}
