struct F
{
	int n;
	char bits[];
};

struct F a = { 0 }; // fine
struct F a = { 0, { 1 }}; // gnu

main()
{
	struct F b;
	struct F c = { 2, { 5, 6 } };

	b.bits[0] = 2; // UB - should warn on auto-allocation of struct F without an init

	return c.bits[1]; // 6
}
