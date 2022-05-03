int printf(const char *, ...);

void f(int i)
{
	printf("%d\n", i);
}

main()
{
	for(int i = 10; i; i--)
		f(i);

  printf("---\n");

	for(int i = 0; i < 10; i++)
		f(i);

  printf("---\n");

	for(int i = 0; i <= 10; i++)
		f(i);

	// TODO: try to convert
	//   sub r0, r0, #1
	//   cmp r0, #0
	//   ble .Lfin
	//   ...
	//
	// to
	//   subs r0, r0, #1
	//   @  ^ set condition codes
	//   ble .Lfin
	//   ...
}

/*
 * [x] eq 	Equal. 	Z==1
 * [x] ne 	Not equal. 	Z==0
 * [x] cs or hs 	Unsigned higher or same (or carry set). 	C==1
 * [x] cc or lo 	Unsigned lower (or carry clear). 	C==0
 * [.] mi 	Negative. The mnemonic stands for "minus". 	N==1
 * [.] pl 	Positive or zero. The mnemonic stands for "plus". 	N==0
 * [.] vs 	Signed overflow. The mnemonic stands for "V set". 	V==1
 * [.] vc 	No signed overflow. The mnemonic stands for "V clear". 	V==0
 * [x] hi 	Unsigned higher. 	(C==1) && (Z==0)
 * [x] ls 	Unsigned lower or same. 	(C==0) || (Z==1)
 * [x] ge 	Signed greater than or equal. 	N==V
 * [x] lt 	Signed less than. 	N!=V
 * [x] gt 	Signed greater than. 	(Z==0) && (N==V)
 * [x] le 	Signed less than or equal. 	(Z==1) || (N!=V)
 * [-] al (or omitted) 	Always executed. 	None tested.
 */
