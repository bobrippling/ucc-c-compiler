#include <dirent.h>

int main()
{
	DIR *d = opendir(".");
	if(d){
		struct dirent *ent;

		while((ent = readdir(d)))
			printf("ent %s\n", ent->d_name);

		closedir(d);
	}
}
