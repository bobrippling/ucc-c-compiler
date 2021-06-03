// RUN: %ocheck 1 %s -fsigned-char
// RUN: %ocheck 0 %s -fno-signed-char
// RUN: %ocheck 1 %s -fno-unsigned-char
// RUN: %ocheck 0 %s -funsigned-char

// check with no constant folding

// RUN: %ocheck 1 %s -fno-const-fold -fsigned-char
// RUN: %ocheck 0 %s -fno-const-fold -fno-signed-char
// RUN: %ocheck 1 %s -fno-const-fold -fno-unsigned-char
// RUN: %ocheck 0 %s -fno-const-fold -funsigned-char


// signed by default - may change when ported
// RUN: %ocheck 1 %s

extern _Noreturn void abort(void);

main()
#include <ocheck-init.c>
{
	int bis = __builtin_is_signed(char);
	int is;

	if((int)(char)-1 == -1){
		is = 1;
	}else if ((int)(char)-1 == 255){
		is = 0;
	}else{
		abort();
	}

	if(is != bis)
		abort();

	return is;
}
