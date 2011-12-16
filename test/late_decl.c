main()
{
	extern const char *strs[];
	int i;

	for(i = 0; strs[i]; i++)
		printf("strs[%d] = %s\n", i, strs[i]);

	return 0;
}

char *strs[] = {
	"hi",
	"there",
	"tim",
	0
};
