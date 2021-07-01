/*
flags=-fpic
for model in global-dynamic initial-exec local-dynamic local-exec
do
	echo "model=$model";
	cc -S -o- a.c -O2 -ftls-model=$model $flags
	clang -S -o- a.c -O2 -ftls-model=$model $flags
done
*/

//extern
__thread int tls0;
__thread int tls1 = 2;

// TODO: -fdata-sections places it into .tbss.$varname
// TODO: __attribute((section(...))) places it into $section,"awT",@progbits
//                                                              ^

int get() {
	return tls1;
}

int *addr() {
	return &tls1;
}

void set(int x) {
	tls1 = x;
}
