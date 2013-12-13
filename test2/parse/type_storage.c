// RUN: %check -e %s

main()
{
	return (static)5; // CHECK: /error: storage unwanted \(static\)/
}
