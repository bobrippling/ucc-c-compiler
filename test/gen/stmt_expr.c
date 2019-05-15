// RUN: %check --only %s
// RUN: %ocheck 0 %s

f(int i)
{
	({
	 (void)0;
	 });
	({
	 return i - 2;
	 });
}

static void assert(char c)
{
	void abort(void);
	if(c == 0)
		abort();
}

int main()
{
	assert(
			({
				3; // CHECK: warning: unused expression
				int x = 3; // force new scope
				5;
			})
			==
			5);

	assert(
			({
				3; // CHECK: unused expression
				({
					5;
				});
			})
			==
			5);

	assert(
			1 + ({
				int i = 5;
				3 + f(i + 1);
			}) + 2
			==
			10);

	_Static_assert(
			__builtin_types_compatible_p(
				__typeof(({
					3; // CHECK: unused expression
					if(1){
						2; // CHECK: unused expression
					} })),
				void),
			"");

	int hit = 0;
	({
		3; // CHECK: unused expression
		if(1){ // a bug in the old code meant non-expression-statements in the last
			hit = 1;
		}
	});
	assert(hit);

	_Static_assert(
			__builtin_types_compatible_p(
				__typeof(({
					3; // CHECK: unused expression
					{
						5; // CHECK: unused expression
					}
				})),
				void),
			"");
}
