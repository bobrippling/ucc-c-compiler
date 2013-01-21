// RUN: %ucc -o %t %s
// RUN: %t hi | sed -n 1p | grep '^1'
// RUN: %t hi | sed -n 2p | grep '^1%t$'
// RUN: %t hi | sed -n 3p | grep '^hi$'

nl()
{
	write(1, "\n", 1);
}

main(int argc, char **argv, char **envp)
{
	write(1, &"0123456789"[argc], 1);
	nl();

	int i;
	for(i = 0; i < argc; i++)
		write(1, argv[i], strlen(argv[i])), nl();
}
