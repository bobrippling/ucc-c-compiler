// RUN: %ucc -S -o- %s -fno-const-fold '-DARG=__builtin_nan("")' | grep ucomisd
// RUN: %ucc -S -o- %s -fno-const-fold '-DARG=3' | grep setne

void f(_Bool);

int main()
{
	f(ARG);
}
