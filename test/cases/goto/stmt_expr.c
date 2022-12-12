// RUN: %check --only -e %s

void f(int, int, int);
void g(const char *);

int main(void)
{
	goto bar; // CHECK: error: goto enters statement expression

	f(12, ({bar: g("hello"); 34;}), 56);

	({
		goto d;
		d:;
		goto out;
	});

	out:

	switch(0){
		case 1:
			({
				case 2: // CHECK: error: case not inside switch
					;
			});
	}

	return 0;
}
