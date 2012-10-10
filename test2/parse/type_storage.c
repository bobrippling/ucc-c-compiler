// RUN: %ucc %s | %check %s

main()
{
	return (static)5; // CHECK: /error: type storage unwanted (static)/
}
