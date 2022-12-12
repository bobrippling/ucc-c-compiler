// RUN: %check --only -e %s -Wno-unused-label -Wno-dead-code

int f(void);

_Noreturn void noreturn(void);

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
			break; // don't interfere with below warnings

		/* shouldn't get a warning on:
		 * break/continue/return/goto before a case/case-range/label
		 */
		case 20: break; case 21: break;
		case 23 ... 27: break; case 28: break;
		hello2: break; case 29: break;

		for(;1;){ // enable 'continue' statement testing
		case 30: continue; case 31: break;
		case 33 ... 37: continue; case 38: break;
		hello3: continue; case 39: break;
		}
		break; // don't interfere with below warnings - can't tell whether the for-loop exits

		case 40: return 3; case 41: break;
		case 43 ... 47: return 3; case 48: break;
		hello4: return 3; case 49: break;

		case 50: goto hello; case 51: break;
		case 53 ... 57: goto hello; case 58: break;
		hello5: goto hello; case 59: break;

		// shouldn't warn for noreturn func, etc
		case 60: noreturn(); case 70: break;
		case 61: __builtin_unreachable(); case 71: break;
	}

	__attribute((fallthrough))
	_Static_assert(0 == 0, ""); // CHECK: error: fallthrough attribute on static-assert

	__attribute((fallthrough))
	hello999: // CHECK: error: fallthrough attribute on label
		;
}
