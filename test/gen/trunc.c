// RUN: %ucc -w -c %s >%t 2>&1
// RUN: test $(wc -c %t) -eq 0
// ^ should assemble, despite 65536 not fitting in a char
//
// RUN: %layout_check %s

char x = 65536;

main()
{
	char x = 65536; // needs truncation in backend
}
