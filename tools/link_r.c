#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>

static int verbose;

static int endswith(const char *haystack, const char *needle)
{
	size_t lh = strlen(haystack);
	size_t ln = strlen(needle);

	if(ln > lh)
		return 0;

	return !strcmp(haystack + (lh - ln), needle);
}

static const char *level_to_dotdot(int level)
{
	static char buf[32];
	int max = sizeof(buf);
	*buf = '\0';

	while(level --> 0){
		if(max < 4){
			fprintf(stderr, "not enough buffer space\n");
			exit(1);
		}

		strcat(buf, "../");
		max -= 3;
	}

	return buf;
}

static int count_dirs(const char *target)
{
	int count = 0;
	int had_nonslash = 0;
	const char *p;

	for(p = target; *p; p++){
		if(*p == '/'){
			if(had_nonslash){
				count++;
				had_nonslash = 0;
			}
		}else{
			had_nonslash = 1;
		}
	}

	return count + had_nonslash;
}

static int link_r(char *src, char *target, int level)
{
	int ret = 0;
	struct dirent *ent;
	DIR *d = opendir(src);

	if(!d){
		fprintf(stderr, "opendir \"%s\": %s\n", src, strerror(errno));
		return 1;
	}

	if(mkdir(target, 0777) && errno != EEXIST){
		fprintf(stderr, "mkdir \"%s\": %s\n", target, strerror(errno));
		ret = 1;
		goto out;
	}
	if(verbose)
		printf("mkdir \"%s\"\n", target);

	while((errno = 0, ent = readdir(d))){
		char there[1024];
		struct stat st;

		if(ent->d_name[0] == '.')
			continue;

		snprintf(there, sizeof(there), "%s/%s", src, ent->d_name);
		if(stat(there, &st)){
			fprintf(stderr, "stat \"%s\": %s\n", there, strerror(errno));
			ret = 1;
			goto out;
		}

		if(S_ISDIR(st.st_mode)){
			char here[1024];
			snprintf(here, sizeof(here), "%s/%s", target, ent->d_name);

			if(link_r(there, here, level + 1)){
				ret = 1;
				goto out;
			}

		}else if(S_ISREG(st.st_mode)){
			char newlink[1024];
			char there_adj[1024];

			if(endswith(ent->d_name, ".o"))
				continue;

			snprintf(there_adj, sizeof(there_adj), "%s%s/%s", level_to_dotdot(count_dirs(target)), src, ent->d_name);
			snprintf(newlink, sizeof(newlink), "%s/%s", target, ent->d_name);

			if(symlink(there_adj, newlink) && errno != EEXIST){
				fprintf(stderr, "symlink \"%s\" --> \"%s\": %s\n", newlink, there_adj, strerror(errno));
				ret = 1;
				goto out;
			}

			if(verbose)
				printf("symlink \"%s\" --> \"%s\" (count_dirs(\"%s\") = %d)\n", newlink, there_adj,
						target, count_dirs(target));

		}else{
			if(verbose)
				printf("ignoring \"%s\"\n", ent->d_name);
		}
	}
	if(errno){
		fprintf(stderr, "readdir \"%s\": %s\n", src, strerror(errno));
		ret = 1;
	}

out:
	if(closedir(d)){
		if(!ret){
			ret = 1;
			fprintf(stderr, "closedir \"%s\": %s\n", src, strerror(errno));
		}
	}

	return ret;
}

static int usage(const char *argv0)
{
	fprintf(stderr, "Usage: %s [-v] [--test] start-path target\n", argv0);
	return 2;
}

static void test(void)
{
	assert(count_dirs("a/b/c") == 3);
	assert(count_dirs("a/b/") == 2);
	assert(count_dirs("a/b") == 2);
	assert(count_dirs("a") == 1);
	assert(count_dirs("") == 0);
	printf("tests passed\n");
}

int main(int argc, char *argv[])
{
	char *src = NULL;
	char *target = NULL;
	int i;

	for(i = 1; i < argc; i++)
		if(!strcmp(argv[i], "-v"))
			verbose = 1;
		else if(!strcmp(argv[i], "--test"))
			test();
		else if(!src)
			src = argv[i];
		else if(!target)
			target = argv[i];
		else
			return usage(argv[0]);

	if(!src || !target)
		return usage(argv[0]);

	return link_r(src, target, 0);
}
