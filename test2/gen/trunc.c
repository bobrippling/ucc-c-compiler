// RUN: %ucc -c %s -o %t 2>&1 | grep '65536'; [ $? -ne 0 ]

main()
{
	char x = 65536; // needs truncation in backend
}
