// RUN: %check --only %s
// RUN: %check -e --only %s -DWITH_ERROR
// RUN: %ocheck 0 %s

int f(int i)
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

int via_goto()
{
  int r = 3;

	r = ({
    int x = 1;
    goto label; // should work
    x;
  });

label:
  return r;
}

void on_cleanup(int *p)
{
	*p = 1;
}

void q(int *p)
{
	*p = 2;
}

int via_cleanup()
{
	return ({
		int x __attribute((cleanup(on_cleanup)));
		q(&x);
		x;
	}) + 3;
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

	// ensure declarations at the end aren't skipped over
	_Static_assert(
			__builtin_types_compatible_p(
				__typeof(({
					3; // CHECK: warning: unused expression (value)
					int q; // CHECK: warning: "q" never written to
				})),
				void),
			"");

	assert(
			({
				// test where the last statement is a label, not an expression
				goto x;
				y: // CHECK: warning: unused label 'y'
				x:
				z: // CHECK: warning: unused label 'z'
				3;
			})
			==
			3);

	assert(via_cleanup() == 5);
	assert(via_goto() == 3);
}
