// RUN: %check %s

int f(int i) // CHECK: warning: control reaches end of non-void function f
{
	switch(i){
		case 1:
			return 2;

		case 2:
			break;

		default:
			return 5;
	}
}
