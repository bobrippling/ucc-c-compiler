// RUN: %check %s -std=c89
// RUN: %check --prefix=error -e %s -DERROR -std=c89
// C89 for implicit function decl

int f()
{
	if(0){
		g(); // CHECK: warning: implicit declaration of function "g"
	}

	__auto_type p = g;
	return p();
}

int h()
{
	return g();
}

#ifdef ERROR
void g(void); // CHECK-error: error: mismatching definitions of "g"
#else
int g(void);
int g();
#endif
