// RUN: %check --only %s -Wno-mismatch-ptr

g();
f();
weak() __attribute((weak));

int glob[3];

int glob_weak[3] __attribute__((weak));

main()
{
	int a[3];
	int h();

	if(g() == 0 && f == 0){ // CHECK: warning: comparison of function with null is always false
	}

	if(a == (void *)0){ //CHECK: warning: comparison of array with null is always false
	}

	if(glob == (char *)0){ // CHECK: warning: comparison of array with null is always false
	}

	if(0 == h){ // CHECK: warning: comparison of function with null is always false
	}

	if(weak == 0){
	}

	if(glob_weak == 0){
	}
}
