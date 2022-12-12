// RUN: %check -e %s -DFAIL
// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

f()
{
	return 7;
}

t_if()
{
	// if can define _one_ variable
	if(int i = f(); i > 5){
		if(i != 7)
			abort();
	}else{
		abort();
	}

	if(int i = f(); i > 8){
		abort();
	}else{
		i;
	}
}

#ifdef FAIL
t_if2()
{
	if(int i = f(), j = 3; j = 2) // CHECK: error: expecting token ')', got ','
		;
}
#endif

t_while()
{
	while(int i = f(); i > 5){
		break;
	}
}

t_switch()
{
	switch(int i = f(); i++)
		break;
}

t_for()
{
	// for() can define multiple variables
	for(int i = 5, j = 2; i + j < 10; j++);
}

main()
{
#include "../ocheck-init.c"
	t_if();
	t_while();
	t_switch();
	t_for();
}
