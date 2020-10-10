// RUN: %check --only %s -Wcomma

void fv(void);
int fi(void);

void f(void){
	fv(), fi(); // CHECK: warning: left hand side of comma is unused
}
