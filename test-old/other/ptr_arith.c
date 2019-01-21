main(int argc, char **argv)
{
	char **arg = argv;
	char *s;

	while(*arg){
		s = *arg;
		while(*s){
			write(1, s, 1);
			s++;
		}
		write(1, "\n", 1);

		++arg;
	}

	return 0;
}
