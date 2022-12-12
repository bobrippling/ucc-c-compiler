// RUN: %check --only %s

void g(void);

_Noreturn void abort(void);

int f(int i) // CHECK: warning: control reaches end of non-void function f
{
	switch(i){
		case 1:
			return 2;

		case 2:
			break; // easy - case --> break, find the break, passable

		case 3: // harder - case --> funcall; break - find the break
			g();
			break;

		default:
			return 5;
	}
}

int f2(int i) // CHECK: warning: control reaches end of non-void function f2
{
	switch(i){
		case 1:
			return 2;

		case 3: // harder - case --> funcall; break - find the break
			abort();
			break; // CHECK: warning: code will never be executed

		default:
			return 5;
	}
}
