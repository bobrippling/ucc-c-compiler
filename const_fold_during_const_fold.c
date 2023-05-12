typedef unsigned long size_t;
char *strcpy(char *, const char *);
void *alloca(size_t);
size_t strlen(const char *);

#define strdupa(strdupa_is_unsafe) \
	strcpy( \
		(char *)alloca(strlen(strdupa_is_unsafe)+1), \
		((strdupa_is_unsafe)?0:(long)(strdupa_is_unsafe)<<-1,(strdupa_is_unsafe)) \
	)

int main() {
	const char *p= strdupa("hi");
}
