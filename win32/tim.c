#define MB_OK 0

main()
{
	char *buf = "Text";

	MessageBoxA(0, buf, "Caption", MB_OK);

	return 5;
}
