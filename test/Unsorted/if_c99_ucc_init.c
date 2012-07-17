int main(int argc, char **argv)
{
	char *a, *b;

	a = "hi", b = "yo";

	if(char *s = a, *s2 = b){
		printf("got s = %s, s2 = %s\n", s, s2);
	}

	return 0;
}
