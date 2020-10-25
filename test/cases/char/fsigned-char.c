// RUN: %ucc -o %t %s -fsigned-char      && %ocheck 1 %t
// RUN: %ucc -o %t %s -fno-signed-char   && %ocheck 0 %t
// RUN: %ucc -o %t %s -fno-unsigned-char && %ocheck 1 %t
// RUN: %ucc -o %t %s -funsigned-char    && %ocheck 0 %t

// check with no constant folding

// RUN: %ucc -o %t %s -fno-const-fold -fsigned-char      && %ocheck 1 %t
// RUN: %ucc -o %t %s -fno-const-fold -fno-signed-char   && %ocheck 0 %t
// RUN: %ucc -o %t %s -fno-const-fold -fno-unsigned-char && %ocheck 1 %t
// RUN: %ucc -o %t %s -fno-const-fold -funsigned-char    && %ocheck 0 %t


// signed by default - may change when ported
// RUN: %ocheck 1 %s

extern _Noreturn void abort(void);

main()
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
