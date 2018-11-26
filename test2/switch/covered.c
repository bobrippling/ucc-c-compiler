// RUN: %check %s -Wcovered-switch-default

enum E
{
  A, B, C
};

f(enum E x)
{
  switch(x){
	  case A:
	  case B:
	  case C:
	  default: // CHECK: warning: default label in switch covering all enum values
				break;
  }
}
