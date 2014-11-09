// RUN: %check %s -Woverflow
main()
{
	char x = 65536; // CHECK: warning: implicit cast changes value from 65536 to 0
}
