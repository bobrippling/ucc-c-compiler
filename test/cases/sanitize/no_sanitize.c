// RUN: %ucc -target x86_64-linux -S -o- %s -fsanitize=undefined | %stdoutcheck %s
// RUN: %check --only %s

__attribute__((no_sanitize("alignment", "object-sizeabc", "undefined"))) // CHECK: warning: unknown sanitizer "alignment"
// CHECK: ^warning: unknown sanitizer "object-sizeabc"
__attribute__((no_sanitize("alignment,object-size"))) // CHECK: warning: unknown sanitizer "alignment"
// CHECK: ^warning: unknown sanitizer "object-size"
int f(int x) {
	// STDOUT:      f:
	// STDOUT-NOT:  ud2
	// STDOUT:      Lfuncend_f:
	return 1 << x;
}

__attribute__((no_sanitize_undefined))
int g(int x) {
	// STDOUT:      g:
	// STDOUT-NOT:  ud2
	// STDOUT:      Lfuncend_g:
	return 1 << x;
}

int h(int x) {
	// STDOUT:  h:
	// STDOUT:  ud2
	// STDOUT:  Lfuncend_h:
	return 1 << x;
}
