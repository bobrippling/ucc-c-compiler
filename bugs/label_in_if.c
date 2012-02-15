int main(int argc, char **argv)
{
	int p = 0;

	if(argc == 3)
		goto x;

	if(p)
		x:
			printf("yo\n");

	printf("hai\n");
}
