char a;
int b = 5;

int *pi;

int (*const ((*(c))));
int ***d;
int ((**(*(e))));
int ((**(*f)));
int (**(*g));

int const *const *const *const h;

//void f;
const char *user_str;
const char **argv;
char *const editable_str;

void *i;
void (*j);
void (*k)();
void (*l)(void);

void  (*m)(int i, int j);
void *(*n)(int);

void func_unspec();
void func(void);
void (*pfunc_unspec)();
void (*pfunc)(void);

int const static *(*const (*o)(void *, void **, void ***))(char *(*f));

//int (*const ((*(n))(*,*))(((*()))))(*);
main()
{
	return 0;
}
