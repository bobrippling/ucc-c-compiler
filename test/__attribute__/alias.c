// RUN: %ucc -S -o %t %s -target x86_64-linux
// RUN: grep 'f = f_impl' %t

/*#define weak __attribute__((__weak__))*/
/*#define weak_alias(old, new) \
  extern __typeof(old) new __attribute__((__weak__, __alias__(#old)))*/

#define alias(old, new) \
  extern __typeof(old) new __attribute__((alias(#old)))

int f_impl()
{
	printf("%s()\n", __func__);
	return 3;
}

/*weak_alias(f_impl, f);*/
alias(f_impl, f);

main()
{
	f();
}
