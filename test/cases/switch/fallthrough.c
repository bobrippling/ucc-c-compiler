// RUN: %check --only -e %s -Wno-unused-label

int f(void);

int main()
{
	switch(f()){
		case 1:
			f();
			__attribute((fallthrough));

		case 2:
			__attribute((fallthrough));
		case 3:
		case 4:
			;
			__attribute((fallthrough)); // CHECK: error: fallthrough statement not directly before switch label
			f();

			// same but should warn:

		case 5: // CHECK: warning: implicit fallthrough between switch labels
			f();

		case 7: // CHECK: warning: implicit fallthrough between switch labels
		case 8:
			;

			// should warn for case-range
		case 9 ... 10: // CHECK: warning: implicit fallthrough between switch labels
			;

			// should warn for defaults too
		default: // CHECK: warning: implicit fallthrough between switch labels
			;

			// but not labels
		hello:
			;
	}

	__attribute((fallthrough))
	_Static_assert(0 == 0, ""); // CHECK: error: fallthrough attribute on static-assert

	__attribute((fallthrough))
	hello2: // CHECK: error: fallthrough attribute on label
		;
}
