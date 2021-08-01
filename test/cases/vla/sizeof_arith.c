// RUN: %ucc -S -o- -target x86_64-linux %s -Dunsigned= | %stdoutcheck --prefix=64-signed   %s
// RUN: %ucc -S -o- -target x86_64-linux %s             | %stdoutcheck --prefix=64-unsigned %s
// TODO %ucc -S -o- -target x86-linux    %s -Dunsigned= | %stdoutcheck --prefix=32-signed   %s
// TODO %ucc -S -o- -target x86-linux    %s             | %stdoutcheck --prefix=32-unsigned %s

// STDOUT-64-unsigned: imulq $8, %rax
// STDOUT-64-signed: imulq $8, %rax

// STDOUT-32-unsigned: imull $3, %eax // 3?
// STDOUT-32-signed: imull $3, %eax

f(unsigned char x)
{
	// sizeof(long) * (x + 2) should be evaluated using size_t arith
	return sizeof(unsigned long[x + 2]);
}

main()
{
	return f(2);
}
