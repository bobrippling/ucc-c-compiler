// RUN: %check --only %s -Wshadow=compatible-local -Wno-sym-never-written

void f()
{
	int i; // no warning here
	{
		char i; // CHECK: note: local declaration here
		{
			char i; // CHECK: warning: declaration of "i" shadows local declaration
		}
	}
}
