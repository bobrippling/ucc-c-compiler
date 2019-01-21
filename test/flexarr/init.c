// RUN: %check %s
// RUN: %ocheck 39 %s
struct F
{
	int n;
	char bits[];
};

struct F a = { 8 }; // CHECK: !/warn/
struct F b = { 8, { 1 }}; // CHECK: /warning: initialisation of flexible array/

main()
{
	static struct F c = { 2, { 5, 6 } }; // CHECK: /warning: initialisation of flexible array/
	struct F local = { 9 };

	//b.bits[0] = 2; // UB

	return
		a.n +
		b.n +
		b.bits[0] +
		c.n +
		c.bits[0] +
		c.bits[1] +
		local.n;
}
