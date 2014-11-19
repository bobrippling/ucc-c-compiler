// RUN: %check %s -Wsigned-unsigned
typedef int sint;
typedef unsigned uint;

main()
{
	__attribute__((unused)) sint a = -1UL; // CHECK: /warning: implicit cast negates value, 18446744073709551615 to -1/
	__attribute__((unused)) sint b = ~0UL; // CHECK: /warning: implicit cast negates value, 18446744073709551615 to -1/
	__attribute__((unused)) uint c = -1UL; // CHECK: /warning: implicit cast truncates value from 18446744073709551615 to 4294967295/
	__attribute__((unused)) uint d = ~0UL; // CHECK: /warning: implicit cast truncates value from 18446744073709551615 to 4294967295/
}
