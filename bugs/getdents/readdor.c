#include <dirent.h>
main()
{
	DIR *d = opendir(".");
	struct dirent *dp;
	while(dp = readdir(d));
	closedir(d);
}
