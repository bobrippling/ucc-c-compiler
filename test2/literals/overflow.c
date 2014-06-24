// RUN: %check %s
main()
{
	char x = 65536; // CHECK: warning: implicit cast changes value from 65536 to 0
}
