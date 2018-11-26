#include <stdio.h>
#include <string.h>
#include <errno.h>

int main(int argc, const char *argv[])
{
	int i;

	if(argc != 1){
		fprintf(stderr, "Usage: %s\n", *argv);
		return 1;
	}

	for(i = 1; i < 1000; i++){
		const char *err;

		errno = 0;
		err = strerror(i);
		if(!err || errno){
			if(errno != EINVAL){
				perror("strerror()");
				return 1;
			}
			break;
		}else if(!strncmp(err, "Unknown error", 13)){
			break;
		}

		printf("\t\"%s\",\n", err);
	}

	return 0;
}
