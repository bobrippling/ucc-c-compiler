// RUN: %ucc -S %s -o %t
// RUN: %ucc -c -x __asm %t 2>&1 | grep '65536'; [ $? -ne 0 ]
// RUN: %layout_check %s

char x = 65536;

main()
{
	char x = 65536; // needs truncation in backend
}
