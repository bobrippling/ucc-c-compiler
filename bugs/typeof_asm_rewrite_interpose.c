#define QUOTE_(x) #x
#define QUOTE(x) QUOTE_(x)

inline void f(int a, char *);

__typeof(f) orig_f asm(QUOTE(__USER_LABEL_PREFIX__) "f");

inline void f(int a, char *p) asm(QUOTE(__USER_LABEL_PREFIX__) "f_wrapper_thunk");
inline void f(int a, char *p)
{
	orig_f(a+1, p+1);
}

int main()
{
	f(0, "hi");
}
